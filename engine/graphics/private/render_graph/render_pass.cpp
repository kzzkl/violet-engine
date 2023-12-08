#include "graphics/render_graph/render_pass.hpp"
#include "common/log.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <cassert>

namespace violet
{
render_attachment::render_attachment(std::size_t index) noexcept : m_desc{}, m_index(index)
{
}

void render_attachment::set_format(rhi_resource_format format) noexcept
{
    m_desc.format = format;
}

void render_attachment::set_load_op(rhi_attachment_load_op op) noexcept
{
    m_desc.load_op = op;
}

void render_attachment::set_store_op(rhi_attachment_store_op op) noexcept
{
    m_desc.store_op = op;
}

void render_attachment::set_stencil_load_op(rhi_attachment_load_op op) noexcept
{
    m_desc.stencil_load_op = op;
}

void render_attachment::set_stencil_store_op(rhi_attachment_store_op op) noexcept
{
    m_desc.stencil_store_op = op;
}

void render_attachment::set_initial_state(rhi_resource_state state) noexcept
{
    m_desc.initial_state = state;
}

void render_attachment::set_final_state(rhi_resource_state state) noexcept
{
    m_desc.final_state = state;
}

render_subpass::render_subpass(std::string_view name, renderer* renderer)
    : render_node(name, renderer),
      m_desc{},
      m_index(0)
{
}

void render_subpass::add_reference(
    render_attachment* attachment,
    rhi_attachment_reference_type type,
    rhi_resource_state state)
{
    auto& desc = m_desc.references[m_desc.reference_count];
    desc.type = type;
    desc.state = state;
    desc.index = attachment->get_index();
    desc.resolve_index = 0;

    ++m_desc.reference_count;
}

void render_subpass::add_reference(
    render_attachment* attachment,
    rhi_attachment_reference_type type,
    rhi_resource_state state,
    render_attachment* resolve)
{
    auto& desc = m_desc.references[m_desc.reference_count];
    desc.type = type;
    desc.state = state;
    desc.index = attachment->get_index();
    desc.resolve_index = resolve->get_index();

    ++m_desc.reference_count;
}

render_pipeline* render_subpass::get_pipeline(std::string_view name) const
{
    for (auto& pipeline : m_pipelines)
    {
        if (pipeline->get_name() == name)
            return pipeline.get();
    }
    return nullptr;
}

bool render_subpass::compile(rhi_render_pass* render_pass, std::size_t index)
{
    for (auto& pipeline : m_pipelines)
    {
        if (!pipeline->compile(render_pass, index))
            return false;
    }
    m_index = index;

    return true;
}

void render_subpass::execute(
    rhi_render_command* command,
    rhi_parameter* camera,
    rhi_parameter* light)
{
    for (auto& pipeline : m_pipelines)
        pipeline->execute(command, camera, light);
}

render_pass::render_pass(std::string_view name, renderer* renderer)
    : render_node(name, renderer),
      m_interface(nullptr)
{
}

render_pass::~render_pass()
{
}

render_attachment* render_pass::add_attachment(std::string_view name)
{
    auto attachment = std::make_unique<render_attachment>(m_attachments.size());
    m_attachments.push_back(std::move(attachment));
    return m_attachments.back().get();
}

render_subpass* render_pass::add_subpass(std::string_view name)
{
    m_subpasses.push_back(std::make_unique<render_subpass>(name, get_renderer()));
    return m_subpasses.back().get();
}

render_pipeline* render_pass::get_pipeline(std::string_view name) const
{
    for (auto& subpass : m_subpasses)
    {
        render_pipeline* pipeline = subpass->get_pipeline(name);
        if (pipeline)
            return pipeline;
    }
    return nullptr;
}

void render_pass::add_dependency(
    render_subpass* source,
    rhi_pipeline_stage_flags source_stage,
    rhi_access_flags source_access,
    render_subpass* target,
    rhi_pipeline_stage_flags target_stage,
    rhi_access_flags target_access)
{
    rhi_render_subpass_dependency_desc dependency = {};
    dependency.source = source == nullptr ? RHI_RENDER_SUBPASS_EXTERNAL : source->get_index();
    dependency.source_stage = source_stage;
    dependency.source_access = source_access;
    dependency.target = target == nullptr ? RHI_RENDER_SUBPASS_EXTERNAL : target->get_index();
    dependency.target_stage = target_stage;
    dependency.target_access = target_access;
    m_dependencies.push_back(dependency);
}

void render_pass::add_camera(
    rhi_scissor_rect scissor,
    rhi_viewport viewport,
    rhi_parameter* parameter,
    rhi_framebuffer* framebuffer)
{
    return m_cameras.push_back({scissor, viewport, parameter, framebuffer});
}

bool render_pass::compile()
{
    rhi_render_pass_desc desc = {};

    for (std::size_t i = 0; i < m_attachments.size(); ++i)
        desc.attachments[i] = m_attachments[i]->get_desc();
    desc.attachment_count = m_attachments.size();

    for (std::size_t i = 0; i < m_subpasses.size(); ++i)
        desc.subpasses[i] = m_subpasses[i]->get_desc();
    desc.subpass_count = m_subpasses.size();

    for (std::size_t i = 0; i < m_dependencies.size(); ++i)
        desc.dependencies[i] = m_dependencies[i];
    desc.dependency_count = m_dependencies.size();

    m_interface = get_renderer()->create_render_pass(desc);

    if (m_interface == nullptr)
        return false;

    for (std::size_t i = 0; i < m_subpasses.size(); ++i)
    {
        if (!m_subpasses[i]->compile(m_interface.get(), i))
            return false;
    }

    return true;
}

void render_pass::execute(rhi_render_command* command, rhi_parameter* light)
{
    assert(!m_attachments.empty());

    for (auto& camera : m_cameras)
    {
        command->begin(m_interface.get(), camera.framebuffer);
        command->set_scissor(&camera.scissor, 1);
        command->set_viewport(camera.viewport);

        for (std::size_t i = 0; i < m_subpasses.size(); ++i)
        {
            m_subpasses[i]->execute(command, camera.parameter, light);
            if (i != m_subpasses.size() - 1)
                command->next();
        }

        command->end();
    }

    m_cameras.clear();
}
} // namespace violet
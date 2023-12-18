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

render_subpass::render_subpass(render_pass* render_pass, std::size_t index)
    : m_desc{},
      m_render_pass(render_pass),
      m_index(index)
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
    auto iter = m_pipeline_map.find(name.data());
    if (iter != m_pipeline_map.end())
        return iter->second;
    else
        return nullptr;
}

bool render_subpass::compile(compile_context& context)
{
    for (auto& pipeline : m_pipelines)
    {
        pipeline->set_render_pass(m_render_pass->get_interface(), m_index);
        if (!pipeline->compile(context))
            return false;
    }

    return true;
}

void render_subpass::execute(execute_context& context)
{
    for (auto& pipeline : m_pipelines)
        pipeline->execute(context);
}

render_pass::render_pass() : m_interface(nullptr)
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
    m_subpasses.push_back(std::make_unique<render_subpass>(this, m_subpasses.size()));
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
    render_subpass* src,
    rhi_pipeline_stage_flags src_stage,
    rhi_access_flags src_access,
    render_subpass* dst,
    rhi_pipeline_stage_flags dst_stage,
    rhi_access_flags dst_access)
{
    rhi_render_subpass_dependency_desc dependency = {};
    dependency.src = src == nullptr ? RHI_RENDER_SUBPASS_EXTERNAL : src->get_index();
    dependency.src_stage = src_stage;
    dependency.src_access = src_access;
    dependency.dst = dst == nullptr ? RHI_RENDER_SUBPASS_EXTERNAL : dst->get_index();
    dependency.dst_stage = dst_stage;
    dependency.dst_access = dst_access;
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

bool render_pass::compile(compile_context& context)
{
    assert(!m_subpasses.empty());

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

    m_interface = context.renderer->create_render_pass(desc);

    if (m_interface == nullptr)
        return false;

    for (std::size_t i = 0; i < m_subpasses.size(); ++i)
    {
        if (!m_subpasses[i]->compile(context))
            return false;
    }

    return true;
}

void render_pass::execute(execute_context& context)
{
    assert(!m_attachments.empty());

    for (auto& camera : m_cameras)
    {
        context.command->begin(m_interface.get(), camera.framebuffer);
        context.command->set_scissor(&camera.scissor, 1);
        context.command->set_viewport(camera.viewport);
        context.camera = camera.parameter;

        for (std::size_t i = 0; i < m_subpasses.size(); ++i)
        {
            m_subpasses[i]->execute(context);
            if (i != m_subpasses.size() - 1)
                context.command->next();
        }

        context.command->end();
    }

    m_cameras.clear();
}
} // namespace violet
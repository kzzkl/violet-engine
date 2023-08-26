#include "graphics/render_graph/render_pass.hpp"
#include "common/hash.hpp"
#include "common/log.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <cassert>

namespace violet
{
render_attachment::render_attachment(std::size_t index, render_resource& resource)
    : m_desc{},
      m_index(index),
      m_resource(resource)
{
    m_desc.format = resource.get_format();
}

void render_attachment::set_load_op(rhi_attachment_load_op op)
{
    m_desc.load_op = op;
}

void render_attachment::set_store_op(rhi_attachment_store_op op)
{
    m_desc.store_op = op;
}

void render_attachment::set_stencil_load_op(rhi_attachment_load_op op)
{
    m_desc.stencil_load_op = op;
}

void render_attachment::set_stencil_store_op(rhi_attachment_store_op op)
{
    m_desc.stencil_store_op = op;
}

void render_attachment::set_initial_state(rhi_resource_state state)
{
    m_desc.initial_state = state;
}

void render_attachment::set_final_state(rhi_resource_state state)
{
    m_desc.final_state = state;
}

render_subpass::render_subpass(
    std::string_view name,
    rhi_context* rhi,
    render_pass* render_pass,
    std::size_t index)
    : render_node(name, rhi),
      m_desc{},
      m_render_pass(render_pass),
      m_index(index)
{
}

void render_subpass::add_reference(
    render_attachment& attachment,
    rhi_attachment_reference_type type,
    rhi_resource_state state)
{
    auto& desc = m_desc.references[m_desc.reference_count];
    desc.type = type;
    desc.state = state;
    desc.index = attachment.get_index();
    desc.resolve_index = 0;

    ++m_desc.reference_count;
}

void render_subpass::add_reference(
    render_attachment& attachment,
    rhi_attachment_reference_type type,
    rhi_resource_state state,
    render_attachment& resolve)
{
    auto& desc = m_desc.references[m_desc.reference_count];
    desc.type = type;
    desc.state = state;
    desc.index = attachment.get_index();
    desc.resolve_index = resolve.get_index();

    ++m_desc.reference_count;
}

render_pipeline& render_subpass::add_pipeline(std::string_view name)
{
    auto pipeline = std::make_unique<render_pipeline>(name, get_rhi());
    m_pipelines.push_back(std::move(pipeline));
    return *m_pipelines.back();
}

bool render_subpass::compile()
{
    for (auto& pipeline : m_pipelines)
    {
        if (!pipeline->compile(m_render_pass->get_interface(), m_index))
            return false;
    }

    return true;
}

void render_subpass::execute(rhi_render_command* command)
{
    for (auto& pipeline : m_pipelines)
        pipeline->execute(command);
}

render_pass::render_pass(std::string_view name, rhi_context* rhi)
    : render_node(name, rhi),
      m_interface(nullptr)
{
}

render_pass::~render_pass()
{
    rhi_context* rhi = get_rhi();

    if (m_interface != nullptr)
        rhi->destroy_render_pass(m_interface);

    for (auto& [key, value] : m_framebuffer_cache)
        rhi->destroy_framebuffer(value);
}

render_attachment& render_pass::add_attachment(std::string_view name, render_resource& resource)
{
    auto attachment = std::make_unique<render_attachment>(m_attachments.size(), resource);
    m_attachments.push_back(std::move(attachment));
    return *m_attachments.back();
}

render_subpass& render_pass::add_subpass(std::string_view name)
{
    auto subpass = std::make_unique<render_subpass>(name, get_rhi(), this, m_subpasses.size());
    m_subpasses.push_back(std::move(subpass));
    return *m_subpasses.back();
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

    m_interface = get_rhi()->make_render_pass(desc);

    if (m_interface == nullptr)
        return false;

    for (auto& subpass : m_subpasses)
    {
        if (!subpass->compile())
            return false;
    }

    return true;
}

void render_pass::execute(rhi_render_command* command)
{
    assert(!m_attachments.empty());

    update_framebuffer_cache();

    rhi_resource_extent extent = m_attachments[0]->get_resource()->get_extent();
    rhi_scissor_rect scissor =
        {.min_x = 0, .min_y = 0, .max_x = extent.width, .max_y = extent.height};
    command->set_scissor(&scissor, 1);

    rhi_viewport viewport = {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
        .min_depth = 0.0f,
        .max_depth = 1.0f};
    command->set_viewport(viewport);

    command->begin(m_interface, m_framebuffer);

    for (std::size_t i = 0; i < m_subpasses.size(); ++i)
    {
        m_subpasses[i]->execute(command);
        if (i != m_subpasses.size() - 1)
            command->next();
    }

    command->end();
}

void render_pass::update_framebuffer_cache()
{
    std::size_t hash = 0;
    for (auto& attachment : m_attachments)
        hash = hash_combine(hash, attachment->get_resource()->get_hash());

    auto iter = m_framebuffer_cache.find(hash);
    if (iter == m_framebuffer_cache.end())
    {
        rhi_framebuffer_desc desc = {};
        desc.render_pass = m_interface;

        for (std::size_t i = 0; i < m_attachments.size(); ++i)
            desc.attachments[i] = m_attachments[i]->get_resource();
        desc.attachment_count = m_attachments.size();

        m_framebuffer = get_rhi()->make_framebuffer(desc);
        m_framebuffer_cache[hash] = m_framebuffer;
    }
    else
    {
        m_framebuffer = iter->second;
    }
}
} // namespace violet
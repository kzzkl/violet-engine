#include "graphics/render_graph/render_pass.hpp"
#include "graphics/render_graph/render_pipeline.hpp"

namespace violet
{
render_pass::render_pass(std::string_view name, rhi_context* rhi)
    : render_node(name, rhi),
      m_interface(nullptr)
{
}

render_pass::~render_pass()
{
    if (m_interface != nullptr)
        get_rhi()->destroy_render_pass(m_interface);
}

void render_pass::set_attachment_description(std::size_t index, const rhi_attachment_desc& desc)
{
    m_attachment_desc[index] = desc;
}

void render_pass::set_attachment_count(std::size_t count)
{
    m_attachment_desc.resize(count);
}

void render_pass::set_subpass_references(
    std::size_t index,
    const std::vector<rhi_attachment_reference>& references)
{
    m_subpasses[index].references = references;
}

void render_pass::set_subpass_count(std::size_t count)
{
    m_subpasses.resize(count);
}

render_pipeline* render_pass::add_pipeline(std::string_view name, std::size_t subpass)
{
    auto pipeline = std::make_unique<render_pipeline>(name, get_rhi(), this, subpass);
    render_pipeline* result = pipeline.get();
    m_subpasses[subpass].pipelines.push_back(std::move(pipeline));
    return result;
}

void render_pass::remove_pipeline(render_pipeline* pipeline)
{
    auto& pipelines = m_subpasses[pipeline->get_subpass()].pipelines;
    for (auto iter = pipelines.begin(); iter != pipelines.end(); ++iter)
    {
        if (iter->get() == pipeline)
        {
            pipelines.erase(iter);
            return;
        }
    }
}

bool render_pass::compile()
{
    if (m_interface != nullptr)
        get_rhi()->destroy_render_pass(m_interface);

    rhi_render_pass_desc desc = {};

    for (std::size_t i = 0; i < m_attachment_desc.size(); ++i)
        desc.attachments[i] = m_attachment_desc[i];
    desc.attachment_count = m_attachment_desc.size();

    for (std::size_t i = 0; i < m_subpasses.size(); ++i)
    {
        for (std::size_t j = 0; j < m_subpasses[i].references.size(); ++j)
            desc.subpasses[i].references[j] = m_subpasses[i].references[j];

        desc.subpasses[i].reference_count = m_subpasses[i].references.size();
    }
    desc.subpass_count = m_subpasses.size();

    m_interface = get_rhi()->make_render_pass(desc);

    return m_interface != nullptr;
}
} // namespace violet
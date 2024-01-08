#include "graphics/render_graph/pass.hpp"
#include <cassert>

namespace violet
{
setup_context::setup_context(
    std::map<std::string, std::pair<render_pipeline*, rhi_parameter_layout*>>& material_pipelines)
    : m_material_pipelines(material_pipelines)
{
}

void setup_context::register_material_pipeline(
    std::string_view name,
    render_pipeline* pipeline,
    rhi_parameter_layout* layout)
{
    assert(m_material_pipelines.find(name.data()) == m_material_pipelines.end());
    m_material_pipelines[name.data()] = std::make_pair(pipeline, layout);
}

execute_context::execute_context(
    rhi_render_command* command,
    rhi_parameter* light,
    const std::unordered_map<std::string, rhi_parameter*>& cameras)
    : m_command(command),
      m_light(light),
      m_cameras(cameras)
{
}

pass_slot::pass_slot(
    std::string_view name,
    std::size_t index,
    pass_slot_type type,
    bool clear) noexcept
    : m_name(name),
      m_index(index),
      m_type(type),
      m_format(RHI_RESOURCE_FORMAT_UNDEFINED),
      m_samples(RHI_SAMPLE_COUNT_1),
      m_input_layout(RHI_IMAGE_LAYOUT_UNDEFINED),
      m_clear(clear),
      m_image(nullptr),
      m_framebuffer_cache(false)
{
}

bool pass_slot::connect(pass_slot* slot)
{
    if (!slot->m_connections.empty() && slot->m_connections[0]->m_input_layout != m_input_layout)
        return false;

    slot->m_connections.push_back(this);
    return true;
}

void pass_slot::set_image(rhi_image* image, bool framebuffer_cache)
{
    m_image = image;
    m_framebuffer_cache = framebuffer_cache;

    for (auto* slot : m_connections)
        slot->set_image(image, framebuffer_cache);
}

pass::pass(renderer* renderer, setup_context& context)
{
}

pass::~pass()
{
}

pass_slot* pass::add_slot(std::string_view name, pass_slot_type type, bool clear)
{
    assert(get_slot(name) == nullptr);

    m_slots.push_back(std::make_unique<pass_slot>(name, m_slots.size(), type, clear));
    return m_slots.back().get();
}

pass_slot* pass::get_slot(std::string_view name) const
{
    for (auto& slot : m_slots)
    {
        if (slot->get_name() == name)
            return slot.get();
    }
    return nullptr;
}
} // namespace violet
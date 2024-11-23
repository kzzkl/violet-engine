#include "graphics/render_context.hpp"
#include "geometry_manager.hpp"
#include "material_manager.hpp"

namespace violet
{
render_context::render_context()
{
    m_material_manager = std::make_unique<material_manager>();
    m_geometry_manager = std::make_unique<geometry_manager>();

    auto& device = render_device::instance();

    m_global_parameter = device.create_parameter(shader::global);
    m_global_parameter->set_storage(0, m_material_manager->get_material_buffer());
}

render_context::~render_context() {}

render_context& render_context::instance()
{
    static render_context instance;
    return instance;
}

render_scene* render_context::get_scene(std::uint32_t scene_id)
{
    while (scene_id >= m_scenes.size())
    {
        m_scenes.push_back(std::make_unique<render_scene>());
    }

    return m_scenes[scene_id].get();
}

bool render_context::update()
{
    bool need_record = false;

    for (auto& scene : m_scenes)
    {
        need_record = scene->update() || need_record;
    }

    need_record = m_material_manager->update() || need_record;

    return need_record;
}

void render_context::record(rhi_command* command)
{
    for (auto& scene : m_scenes)
    {
        scene->record(command);
    }

    m_material_manager->record(command);
}

void render_context::reset()
{
    m_material_manager = nullptr;
    m_geometry_manager = nullptr;

    m_scenes.clear();

    m_global_parameter = nullptr;
}
} // namespace violet
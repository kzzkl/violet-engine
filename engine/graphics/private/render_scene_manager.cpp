#include "render_scene_manager.hpp"

namespace violet
{
bool render_scene_manager::update()
{
    bool need_record = false;
    for (auto& scene : m_scenes)
    {
        need_record = scene->update() || need_record;
    }

    return need_record;
}

void render_scene_manager::record(rhi_command* command)
{
    for (auto& scene : m_scenes)
    {
        scene->record(command);
    }
}

render_scene* render_scene_manager::get_scene(std::uint32_t layer)
{
    assert(layer < 64);

    while (layer >= m_scenes.size())
    {
        m_scenes.push_back(std::make_unique<render_scene>());
    }

    return m_scenes[layer].get();
}
} // namespace violet
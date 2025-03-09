#include "render_scene_manager.hpp"

namespace violet
{
void render_scene_manager::update(gpu_buffer_uploader* uploader)
{
    for (auto& scene : m_scenes)
    {
        scene->update(uploader);
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
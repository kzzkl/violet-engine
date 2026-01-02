#include "render_scene_manager.hpp"

namespace violet
{
render_scene_manager::render_scene_manager(vsm_manager* vsm_manager)
    : m_vsm_manager(vsm_manager)
{
}

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
        m_scenes.push_back(std::make_unique<render_scene>(m_vsm_manager));
    }

    return m_scenes[layer].get();
}
} // namespace violet
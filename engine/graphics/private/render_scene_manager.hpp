#pragma once

#include "graphics/render_scene.hpp"

namespace violet
{
class render_scene_manager
{
public:
    render_scene_manager(vsm_manager* vsm_manager);

    void update(gpu_buffer_uploader* uploader);

    render_scene* get_scene(std::uint32_t layer);

private:
    std::vector<std::unique_ptr<render_scene>> m_scenes;

    vsm_manager* m_vsm_manager;
};
} // namespace violet
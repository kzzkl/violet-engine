#pragma once

#include "graphics/render_scene.hpp"

namespace violet
{
class render_scene_manager
{
public:
    void update(gpu_buffer_uploader* uploader);

    render_scene* get_scene(std::uint32_t layer);

private:
    std::vector<std::unique_ptr<render_scene>> m_scenes;
};
} // namespace violet
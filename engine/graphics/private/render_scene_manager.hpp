#pragma once

#include "graphics/render_scene.hpp"

namespace violet
{
class render_scene_manager
{
public:
    bool update();
    void record(rhi_command* command);

    render_scene* get_scene(std::uint32_t layer);

private:
    std::vector<std::unique_ptr<render_scene>> m_scenes;
};
} // namespace violet
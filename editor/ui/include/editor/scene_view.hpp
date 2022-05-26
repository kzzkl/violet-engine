#pragma once

#include "ecs/entity.hpp"
#include "graphics_interface.hpp"
#include <memory>

namespace ash::editor
{
class scene_view
{
public:
    scene_view(ecs::entity ui_parent);

    void tick();
    void resize();

private:
    void update_camera();
    void resize_camera();

    ecs::entity m_camera;
    std::unique_ptr<graphics::resource> m_render_target;
    std::unique_ptr<graphics::resource> m_render_target_resolve;
    std::unique_ptr<graphics::resource> m_depth_stencil_buffer;

    float m_camera_move_speed;
    float m_camera_rotate_speed;

    std::uint32_t m_width;
    std::uint32_t m_height;

    ecs::entity m_ui;
};
} // namespace ash::editor
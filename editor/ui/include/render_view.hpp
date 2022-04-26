#pragma once

#include "editor_component.hpp"
#include "graphics.hpp"

namespace ash::editor
{
class render_view : public editor_view
{
public:
    render_view(core::context* context);
    virtual ~render_view() = default;

    virtual void draw(editor_data& data) override;

private:
    void update_camera();
    void resize_target();

    void draw_grid();

    std::uint32_t m_target_width;
    std::uint32_t m_target_height;

    bool m_resize_flag;

    ecs::entity m_scene_camera;
    float m_camera_move_speed{1.0f};
    float m_camera_rotate_speed{0.1f};

    std::unique_ptr<graphics::resource> m_render_target;
    std::unique_ptr<graphics::resource> m_depth_stencil;
};
} // namespace ash::editor
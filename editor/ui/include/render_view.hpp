#pragma once

#include "editor_component.hpp"
#include "relation.hpp"
#include "scene.hpp"

namespace ash::editor
{
class render_view : public editor_view
{
public:
    render_view(
        graphics::graphics& graphics,
        ecs::world& world,
        core::relation& relation,
        scene::scene& scene);
    virtual ~render_view() = default;

    virtual void draw(ui::ui& ui, editor_data& data) override;

private:
    void resize_target();

    std::uint32_t m_target_width;
    std::uint32_t m_target_height;

    bool m_resize_flag;

    ecs::entity m_scene_camera;
    std::unique_ptr<graphics::resource> m_render_target;
    std::unique_ptr<graphics::resource> m_depth_stencil;

    graphics::graphics& m_graphics;
    ecs::world& m_world;
    core::relation& m_relation;
    scene::scene& m_scene;
};
} // namespace ash::editor
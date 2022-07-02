#pragma once

#include "core/context.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "graphics/geometry.hpp"
#include "graphics/standard_pipeline.hpp"
#include "scene/bounding_box.hpp"
#include "scene/bvh_tree.hpp"
#include "scene/transform.hpp"
#include "ui/controls/button.hpp"

namespace ash::sample
{
class bvh_viewer : public core::system_base
{
public:
    bvh_viewer();

    virtual bool initialize(const dictionary& config);

private:
    void initialize_graphics_resource();
    void initialize_ui();
    void initialize_task();

    void draw_aabb();
    void update_camera();

    void resize_camera(std::uint32_t width, std::uint32_t height);
    void add_cube(bool random);
    void remove_cube();

    std::vector<std::pair<ecs::entity, std::size_t>> m_cubes;

    ecs::view<scene::transform, scene::bounding_box>* m_aabb_view;

    ecs::entity m_camera;
    float m_heading = 0.0f;
    float m_pitch = 0.0f;
    float m_rotate_speed = 0.8f;
    float m_move_speed = 7.0f;

    graphics::geometry_data m_cube_mesh_data;

    std::unique_ptr<graphics::resource_interface> m_cube_positon_buffer;
    std::unique_ptr<graphics::resource_interface> m_cube_normal_buffer;
    std::unique_ptr<graphics::resource_interface> m_cube_index_buffer;

    std::unique_ptr<graphics::standard_material_pipeline_parameter> m_cube_material;
    std::unique_ptr<graphics::standard_pipeline> m_pipeline;

    std::unique_ptr<graphics::resource_interface> m_render_target;
    std::unique_ptr<graphics::resource_interface> m_depth_stencil_buffer;

    scene::bvh_tree m_tree;

    std::unique_ptr<ui::button> m_add_button;
    std::unique_ptr<ui::button> m_remove_button;
};
} // namespace ash::sample
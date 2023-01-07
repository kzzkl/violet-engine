#pragma once

#include "core/context.hpp"
#include "ecs/entity.hpp"
#include "ecs/world.hpp"
#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/geometry.hpp"
#include "scene/bounding_box.hpp"
#include "scene/bvh_tree.hpp"
#include "scene/transform.hpp"
#include "ui/controls/button.hpp"
#include "ui/controls/image.hpp"

namespace violet::sample
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

    void move_cube();
    void draw_aabb();
    void update_camera();

    void resize_camera(std::uint32_t width, std::uint32_t height);
    void add_cube(bool random);
    void remove_cube();

    ecs::entity m_light;
    std::vector<ecs::entity> m_cubes;
    std::vector<math::float3> m_move_direction;

    ecs::entity m_camera;
    float m_heading = 0.0f;
    float m_pitch = 0.0f;
    float m_rotate_speed = 0.8f;
    float m_move_speed = 7.0f;

    graphics::geometry_data m_cube_mesh_data;

    std::unique_ptr<graphics::resource_interface> m_cube_positon_buffer;
    std::unique_ptr<graphics::resource_interface> m_cube_normal_buffer;
    std::unique_ptr<graphics::resource_interface> m_cube_index_buffer;

    std::unique_ptr<graphics::blinn_phong_material_pipeline_parameter> m_cube_material;
    std::unique_ptr<graphics::blinn_phong_pipeline> m_pipeline;

    std::unique_ptr<graphics::resource_interface> m_render_target;
    std::unique_ptr<graphics::resource_interface> m_depth_stencil_buffer;

    ecs::entity m_small_camera;
    std::unique_ptr<graphics::resource_interface> m_small_render_target;
    std::unique_ptr<graphics::resource_interface> m_small_render_target_resolve;
    std::unique_ptr<graphics::resource_interface> m_small_depth_stencil_buffer;

    // scene::bvh_tree m_tree;

    std::unique_ptr<ui::button> m_add_button;
    std::unique_ptr<ui::button> m_remove_button;
    std::unique_ptr<ui::image> m_small_view;
};
} // namespace violet::sample
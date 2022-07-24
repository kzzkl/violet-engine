#pragma once

#include "core/context.hpp"
#include "ecs/entity.hpp"
#include "graphics/blinn_phong_pipeline.hpp"
#include "graphics/geometry.hpp"

namespace ash::sample
{
class light_viewer : public core::system_base
{
public:
    light_viewer();

    virtual bool initialize(const dictionary& config) override;

private:
    void initialize_graphics_resource();
    void initialize_task();
    void initialize_scene();

    void update_camera();

    void resize_camera(std::uint32_t width, std::uint32_t height);

    ecs::entity m_cube;
    ecs::entity m_sphere;
    ecs::entity m_plane;
    ecs::entity m_light;

    graphics::geometry_data m_cube_mesh_data;
    std::unique_ptr<graphics::resource_interface> m_cube_positon_buffer;
    std::unique_ptr<graphics::resource_interface> m_cube_normal_buffer;
    std::unique_ptr<graphics::resource_interface> m_cube_index_buffer;

    graphics::geometry_data m_sphere_mesh_data;
    std::unique_ptr<graphics::resource_interface> m_sphere_positon_buffer;
    std::unique_ptr<graphics::resource_interface> m_sphere_normal_buffer;
    std::unique_ptr<graphics::resource_interface> m_sphere_index_buffer;

    ecs::entity m_camera;
    math::float3 m_camera_rotation{};
    float m_rotate_speed = 0.8f;
    float m_move_speed = 7.0f;

    std::unique_ptr<graphics::resource_interface> m_render_target;
    std::unique_ptr<graphics::resource_interface> m_depth_stencil_buffer;

    std::unique_ptr<graphics::blinn_phong_material_pipeline_parameter> m_material;
    std::unique_ptr<graphics::blinn_phong_pipeline> m_pipeline;
};
} // namespace ash::sample
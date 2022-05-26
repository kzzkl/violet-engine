#pragma once

#include "core/context.hpp"
#include "graphics/graphics.hpp"
#include "graphics/standard_pipeline.hpp"
#include "physics/physics.hpp"

namespace ash::editor
{
class test_module : public core::system_base
{
public:
    test_module() : core::system_base("test_module") {}

    virtual bool initialize(const ash::dictionary& config) override;

private:
    std::unique_ptr<physics::collision_shape_interface> m_cube_shape;
    std::unique_ptr<physics::collision_shape_interface> m_plane_shape;

    std::unique_ptr<graphics::standard_pipeline> m_pipeline;

    std::vector<std::unique_ptr<graphics::resource>> m_cube_vertex_buffers;
    std::unique_ptr<graphics::resource> m_cube_index_buffer;
    std::unique_ptr<graphics::pipeline_parameter> m_cube_material;
    std::vector<std::unique_ptr<graphics::pipeline_parameter>> m_cube_object;

    ecs::entity m_cube_1, m_cube_2;
    ecs::entity m_plane;

    float m_heading = 0.0f, m_pitch = 0.0f;

    float m_rotate_speed = 0.2f;
    float m_move_speed = 7.0f;
};
} // namespace ash::editor
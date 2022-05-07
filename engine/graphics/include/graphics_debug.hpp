#pragma once

#include "graphics_interface.hpp"
#include "render_pipeline.hpp"
#include "world.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class debug_pass : public render_pass
{
public:
    debug_pass(render_pass_interface* interface);
    virtual void render(const camera& camera, render_command_interface* command) override;
};

class graphics;
class graphics_debug
{
public:
    struct vertex
    {
        math::float3 position;
        math::float3 color;
    };

public:
    graphics_debug(std::size_t frame_resource, graphics& graphics, ecs::world& world);

    void initialize();
    void sync();

    void begin_frame();
    void end_frame();

    ecs::entity entity() const noexcept { return m_entity; }

    void draw_line(const math::float3& start, const math::float3& end, const math::float3& color);

private:
    ecs::world& m_world;
    ecs::entity m_entity;

    std::vector<vertex> m_vertics;

    std::size_t m_index;
    std::vector<std::unique_ptr<resource_interface>> m_vertex_buffer;
    std::unique_ptr<resource_interface> m_index_buffer;

    std::unique_ptr<debug_pass> m_pipeline;
};
} // namespace ash::graphics
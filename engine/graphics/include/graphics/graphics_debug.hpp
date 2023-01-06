#pragma once

#include "ecs/world.hpp"
#include "graphics/render_pipeline.hpp"
#include "graphics_interface.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class debug_pipeline : public render_pipeline
{
public:
    debug_pipeline();

    virtual void render(const render_context& context, render_command_interface* command) override;

private:
    std::unique_ptr<render_pipeline_interface> m_interface;
};

class graphics_debug
{
public:
    static constexpr std::size_t MAX_VERTEX_COUNT = 4096 * 16;

public:
    graphics_debug(std::size_t frame_resource);

    void initialize();
    void sync();

    void next_frame();

    ecs::entity entity() const noexcept { return m_entity; }

    void draw_line(const math::float3& start, const math::float3& end, const math::float3& color);
    void draw_aabb(const math::float3& min, const math::float3& max, const math::float3& color);

private:
    ecs::entity m_entity;

    std::vector<math::float3> m_vertex_position;
    std::vector<math::float3> m_vertex_color;

    std::vector<std::unique_ptr<resource_interface>> m_vertex_buffers;
    std::unique_ptr<resource_interface> m_index_buffer;

    std::unique_ptr<debug_pipeline> m_pipeline;

    std::size_t m_frame_resource;
};
} // namespace ash::graphics
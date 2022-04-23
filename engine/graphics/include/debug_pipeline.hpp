#pragma once

#include "graphics_interface.hpp"
#include "render_pipeline.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class debug_pipeline : public render_pipeline
{
public:
    struct vertex
    {
        math::float3 position;
        math::float3 color;
    };

public:
    debug_pipeline(
        layout_type* layout,
        pipeline_type* pipeline,
        const std::vector<resource*>& vertex_buffer,
        resource* index_buffer);
    virtual ~debug_pipeline() = default;

    void draw_line(const math::float3& start, const math::float3& end, const math::float3& color);

    resource* vertex_buffer() const noexcept { return m_vertex_buffer[m_index].get(); }
    resource* index_buffer() const noexcept { return m_index_buffer.get(); }

    std::size_t vertex_count() const noexcept { return m_vertics.size(); }

    void reset() noexcept { m_vertics.clear(); }
    bool empty() const noexcept { return m_vertics.empty(); }

    virtual void render(
        resource* target,
        resource* depth_stencil,
        render_command* command,
        render_parameter* pass) override;

private:
    std::vector<vertex> m_vertics;

    std::size_t m_index;
    std::vector<std::unique_ptr<resource>> m_vertex_buffer;
    std::unique_ptr<resource> m_index_buffer;
};
} // namespace ash::graphics
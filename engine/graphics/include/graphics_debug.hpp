#pragma once

#include "graphics_exports.hpp"
#include "graphics_interface.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class GRAPHICS_API graphics_debug
{
public:
    struct vertex
    {
        math::float3 position;
        math::float3 color;
    };

public:
    graphics_debug(const std::vector<resource*>& vertex_buffer, resource* index_buffer);

    void draw_line(const math::float3& start, const math::float3& end, const math::float3& color);

    void sync();

    resource* vertex_buffer() const noexcept { return m_vertex_buffer[m_index].get(); }
    resource* index_buffer() const noexcept { return m_index_buffer.get(); }

    std::size_t vertex_count() const noexcept { return m_vertics.size(); }

    void clear() noexcept { m_vertics.clear(); }
    bool empty() const noexcept { return m_vertics.empty(); }

private:
    std::vector<vertex> m_vertics;

    std::size_t m_index;
    std::vector<std::unique_ptr<resource>> m_vertex_buffer;
    std::unique_ptr<resource> m_index_buffer;
};
} // namespace ash::graphics
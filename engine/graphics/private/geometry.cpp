#include "graphics/geometry.hpp"

namespace violet
{
rhi_resource* geometry::get_attribute(std::string_view name)
{
    auto iter = m_vertex_buffers.find(name.data());
    if (iter != m_vertex_buffers.end())
        return iter->second;
    else
        return nullptr;
}

void add_attribute(
    std::string_view name,
    const void* data,
    std::size_t size,
    rhi_resource_format format)
{
}
} // namespace violet
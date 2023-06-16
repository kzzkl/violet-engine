#pragma once

#include "graphics/rhi.hpp"
#include <string>
#include <unordered_map>

namespace violet
{
class geometry
{
public:
    template <typename T>
    void set_indices(const std::vector<T>& indices)
    {
    }

    template <typename T>
    void add_attribute(
        std::string_view name,
        const std::vector<T>& attribute,
        rhi_resource_format format)
    {
        add_attribute(name, attribute.data(), attribute.size() * sizeof(T), format);
    }

    rhi_resource* get_attribute(std::string_view name);

private:
    void add_attribute(
        std::string_view name,
        const void* data,
        std::size_t size,
        rhi_resource_format format);

    std::unordered_map<std::string, rhi_resource*> m_vertex_buffers;
    rhi_resource* m_index_buffer;
};
} // namespace violet
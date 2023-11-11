#pragma once

#include "graphics/render_interface.hpp"
#include <string>
#include <unordered_map>

namespace violet
{
class geometry
{
public:
    geometry(rhi_renderer* rhi);
    virtual ~geometry();

    template <typename T>
    void add_attribute(
        std::string_view name,
        const std::vector<T>& attribute,
        rhi_buffer_flags flags = RHI_BUFFER_FLAG_VERTEX)
    {
        add_attribute(name, attribute.data(), attribute.size() * sizeof(T), flags);
        m_vertex_count = attribute.size();
    }

    template <typename T>
    void set_indices(const std::vector<T>& indices, rhi_buffer_flags flags = RHI_BUFFER_FLAG_INDEX)
    {
        set_indices(indices.data(), indices.size() * sizeof(T), sizeof(T), flags);
        m_index_count = indices.size();
    }

    rhi_resource* get_vertex_buffer(std::string_view name);
    rhi_resource* get_index_buffer() const noexcept { return m_index_buffer; }

    std::size_t get_vertex_count() const noexcept { return m_vertex_count; }
    std::size_t get_index_count() const noexcept { return m_index_count; }

private:
    void add_attribute(
        std::string_view name,
        const void* data,
        std::size_t size,
        rhi_buffer_flags flags);
    void set_indices(
        const void* data,
        std::size_t size,
        std::size_t index_size,
        rhi_buffer_flags flags);

    std::unordered_map<std::string, rhi_resource*> m_vertex_buffers;
    rhi_resource* m_index_buffer;

    std::size_t m_vertex_count;
    std::size_t m_index_count;

    rhi_renderer* m_rhi;
};
} // namespace violet
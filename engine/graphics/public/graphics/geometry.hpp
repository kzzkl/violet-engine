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
    void add_attribute(std::string_view name, const std::vector<T>& attribute, bool dynamic = false)
    {
        add_attribute(name, attribute.data(), attribute.size() * sizeof(T), dynamic);
    }

    template <typename T>
    void set_indices(const std::vector<T>& indices, bool dynamic = false)
    {
        set_indices(indices.data(), indices.size() * sizeof(T), sizeof(T), dynamic);
    }

    rhi_resource* get_vertex_buffer(std::string_view name);
    rhi_resource* get_index_buffer() const noexcept { return m_index_buffer; }

private:
    void add_attribute(std::string_view name, const void* data, std::size_t size, bool dynamic);
    void set_indices(const void* data, std::size_t size, std::size_t index_size, bool dynamic);

    std::unordered_map<std::string, rhi_resource*> m_vertex_buffers;
    rhi_resource* m_index_buffer;

    rhi_renderer* m_rhi;
};
} // namespace violet
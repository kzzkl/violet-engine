#pragma once

#include "graphics/render_graph/render_node.hpp"
#include <unordered_map>

namespace violet
{
class geometry : public render_node
{
public:
    geometry(render_context* context);
    virtual ~geometry();

    template <typename T>
    void add_attribute(std::string_view name, const std::vector<T>& attribute)
    {
        add_attribute(name, attribute.data(), attribute.size() * sizeof(T));
    }

    template <typename T>
    void set_indices(const std::vector<T>& indices)
    {
        set_indices(indices.data(), indices.size() * sizeof(T), sizeof(T));
    }

    rhi_resource* get_vertex_buffer(std::string_view name);
    rhi_resource* get_index_buffer() const noexcept { return m_index_buffer; }

private:
    void add_attribute(std::string_view name, const void* data, std::size_t size);
    void set_indices(const void* data, std::size_t size, std::size_t index_size);

    std::unordered_map<std::string, rhi_resource*> m_vertex_buffers;
    rhi_resource* m_index_buffer;
};
} // namespace violet
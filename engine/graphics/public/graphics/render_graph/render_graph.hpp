#pragma once

#include "graphics/render_graph/rdg_edge.hpp"
#include "graphics/render_graph/rdg_pass.hpp"
#include "graphics/render_graph/rdg_resource.hpp"
#include <map>
#include <memory>

namespace violet
{
template <typename T>
concept RDGResource = std::is_same_v<T, rdg_texture> || std::is_same_v<T, rdg_buffer>;

template <typename T>
concept RDGPass = std::is_base_of_v<rdg_pass, T>;

class rdg_pass_batch;
class render_graph
{
public:
    render_graph() noexcept;
    render_graph(const render_graph&) = delete;
    ~render_graph();

    template <RDGResource T, typename... Args>
    T* add_resource(std::string_view name, bool external, Args&&... args)
    {
        auto resource = std::make_unique<T>(std::forward<Args>(args)...);
        resource->m_name = name;
        resource->m_external = external;

        T* result = resource.get();
        m_resources.push_back(std::move(resource));
        return result;
    }

    template <RDGPass T, typename... Args>
    T* add_pass(std::string_view name, Args&&... args)
    {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        pass->m_name = name;

        T* result = pass.get();
        m_passes.push_back(std::move(pass));
        return result;
    }

    void add_edge(rdg_pass* src, rdg_pass* dst);
    void add_edge(
        rdg_resource* resource,
        rdg_pass* pass,
        std::string_view reference_name,
        rdg_edge_operate operate = RDG_EDGE_OPERATE_DONT_CARE);
    void add_edge(
        rdg_pass* src,
        std::string_view src_reference_name,
        rdg_pass* dst,
        std::string_view dst_reference_name,
        rdg_edge_operate operate = RDG_EDGE_OPERATE_DONT_CARE);

    void compile(render_device* device);
    void execute(rhi_command* command, rdg_context* context);

    std::unique_ptr<rdg_context> create_context();

    std::size_t get_resource_index(std::string_view name) const;
    rdg_resource_type get_resource_type(std::size_t index) const;

    std::size_t get_pass_index(std::string_view name) const;

    render_graph& operator=(const render_graph&) = delete;

private:
    void bind_resource();
    void dead_stripping();
    std::vector<std::vector<rdg_pass*>> merge_pass();

    std::vector<std::unique_ptr<rdg_resource>> m_resources;
    std::vector<std::unique_ptr<rdg_pass>> m_passes;
    std::vector<std::unique_ptr<rdg_edge>> m_edges;

    std::unordered_map<std::string, std::size_t> m_resource_indices;
    std::unordered_map<std::string, std::size_t> m_pass_indices;

    std::vector<std::unique_ptr<rdg_pass_batch>> m_batchs;

    std::vector<std::vector<rhi_ptr<rhi_semaphore>>> m_semaphores;
};
} // namespace violet
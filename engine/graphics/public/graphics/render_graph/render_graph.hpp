#pragma once

#include "graphics/render_graph/edge.hpp"
#include "graphics/render_graph/pass.hpp"
#include "graphics/render_graph/resource.hpp"
#include "graphics/renderer.hpp"
#include <map>
#include <memory>

namespace violet
{
template <typename T>
concept resource_type =
    std::is_same_v<T, texture> || std::is_same_v<T, buffer> || std::is_same_v<T, swapchain>;

template <typename T>
concept pass_type = std::is_base_of_v<pass, T>;

class pass_batch;
class render_graph
{
public:
    render_graph(renderer* renderer) noexcept;
    render_graph(const render_graph&) = delete;
    ~render_graph();

    template <resource_type T, typename... Args>
    T* add_resource(Args&&... args)
    {
        auto resource = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = resource.get();

        if constexpr (std::is_same_v<texture, T>)
            m_textures.push_back(std::move(resource));
        if constexpr (std::is_same_v<buffer, T>)
            m_buffers.push_back(std::move(resource));
        if constexpr (std::is_same_v<swapchain, T>)
            m_swapchains.push_back(std::move(resource));

        return result;
    }

    template <resource_type T>
    T* get_resource(std::string_view name)
    {
        if constexpr (std::is_same_v<texture, T>)
            return find_resource(name, m_textures);
        if constexpr (std::is_same_v<buffer, T>)
            return find_resource(name, m_buffers);
        if constexpr (std::is_same_v<swapchain, T>)
            return find_resource(name, m_swapchains);

        return nullptr;
    }

    template <pass_type T, typename... Args>
    T* add_pass(std::string_view name, Args&&... args)
    {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        pass->m_name = name;

        if constexpr (std::is_base_of_v<mesh_pass, T>)
            pass->m_flags |= PASS_FLAG_MESH;
        if constexpr (std::is_base_of_v<render_pass, T>)
            pass->m_flags |= PASS_FLAG_RENDER;

        T* result = pass.get();

        m_passes.push_back(std::move(pass));
        return result;
    }

    void add_edge(
        resource* resource,
        pass* pass,
        std::string_view reference_name,
        edge_operate operate = EDGE_OPERATE_DONT_CARE);
    void add_edge(
        pass* src,
        std::string_view src_reference_name,
        pass* dst,
        std::string_view dst_reference_name,
        edge_operate operate = EDGE_OPERATE_DONT_CARE);

    void compile();
    rhi_semaphore* execute();

    render_graph& operator=(const render_graph&) = delete;

private:
    void bind_resource();
    void dead_stripping();
    std::vector<std::vector<pass*>> merge_pass();

    template <typename T>
    T* find_resource(std::string_view name, const std::vector<std::unique_ptr<T>>& resources) const
    {
        auto iter = std::find_if(
            resources.cbegin(),
            resources.cend(),
            [name](const auto& resource)
            {
                return resource->get_name() == name;
            });

        return iter == resources.cend() ? nullptr : iter->get();
    }

    void switch_frame_resource();
    rhi_semaphore* allocate_semaphore();

    std::vector<std::unique_ptr<texture>> m_textures;
    std::vector<std::unique_ptr<buffer>> m_buffers;
    std::vector<std::unique_ptr<swapchain>> m_swapchains;

    std::vector<std::unique_ptr<pass>> m_passes;
    std::vector<std::unique_ptr<edge>> m_edges;

    std::vector<std::unique_ptr<pass_batch>> m_batchs;

    std::vector<std::vector<rhi_semaphore*>> m_used_semaphores;
    std::vector<rhi_semaphore*> m_free_semaphores;
    std::vector<rhi_ptr<rhi_semaphore>> m_semaphores;

    renderer* m_renderer;
};
} // namespace violet
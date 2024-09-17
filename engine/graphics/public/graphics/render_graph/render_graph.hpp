#pragma once

#include "graphics/render_graph/rdg_allocator.hpp"
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

class render_graph
{
public:
    render_graph(rdg_allocator* allocator) noexcept;
    render_graph(const render_graph&) = delete;
    ~render_graph();

    rdg_texture* add_texture(
        std::string_view name,
        rhi_texture* texture,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    rdg_texture* add_texture(
        std::string_view name,
        const rhi_texture_desc& desc,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    rdg_texture* add_texture(
        std::string_view name,
        rhi_texture* texture,
        std::uint32_t level,
        std::uint32_t layer,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    rdg_texture* add_texture(
        std::string_view name,
        rdg_texture* texture,
        std::uint32_t level,
        std::uint32_t layer,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    rdg_buffer* add_buffer(std::string_view name, rhi_buffer* buffer);

    template <RDGPass T, typename... Args>
    T* add_pass(std::string_view name, Args&&... args)
    {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        pass->m_name = name;

        T* result = pass.get();
        m_passes.push_back(std::move(pass));

        m_groups.push_back({});

        return result;
    }

    void begin_group(std::string_view group_name);
    void end_group();

    void compile();
    void execute(rhi_command* command);

    render_graph& operator=(const render_graph&) = delete;

public:
    template <typename T>
    T& allocate_data()
    {
        return m_allocator->allocate_data<T>();
    }

    rhi_parameter* allocate_parameter(const rhi_parameter_desc& desc)
    {
        return m_allocator->allocate_parameter(desc);
    }

    rhi_sampler* allocate_sampler(const rhi_sampler_desc& desc)
    {
        return m_allocator->get_sampler(desc);
    }

private:
    void cull();
    void merge_pass();
    void build_barriers();

    std::vector<std::unique_ptr<rdg_resource>> m_resources;
    std::vector<std::unique_ptr<rdg_pass>> m_passes;

    std::vector<std::vector<std::string>> m_groups;

    struct barrier
    {
        rhi_pipeline_stage_flags src_stages;
        rhi_pipeline_stage_flags dst_stages;

        std::vector<rhi_texture_barrier> texture_barriers;
        std::vector<rhi_buffer_barrier> buffer_barriers;
    };
    std::vector<barrier> m_barriers;

    struct render_batch
    {
        std::vector<rdg_pass*> passes;

        rhi_render_pass* render_pass;
        rhi_framebuffer* framebuffer;
    };
    std::vector<render_batch> m_batches;

    rdg_allocator* m_allocator{nullptr};
};

class rdg_scope
{
public:
    rdg_scope(render_graph& graph, std::string_view name)
        : m_graph(graph)
    {
        m_graph.begin_group(name);
    }

    ~rdg_scope()
    {
        m_graph.end_group();
    }

private:
    render_graph& m_graph;
};

} // namespace violet
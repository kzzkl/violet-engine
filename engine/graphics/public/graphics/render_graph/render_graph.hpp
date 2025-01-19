#pragma once

#include "graphics/render_graph/rdg_allocator.hpp"
#include "graphics/render_graph/rdg_pass.hpp"
#include "graphics/render_graph/rdg_resource.hpp"

namespace violet
{
class render_graph
{
public:
    render_graph(std::string_view name, rdg_allocator* allocator) noexcept;
    render_graph(const render_graph&) = delete;
    ~render_graph();

    rdg_texture* add_texture(
        std::string_view name,
        rhi_texture* texture,
        rhi_texture_layout initial_layout = RHI_TEXTURE_LAYOUT_UNDEFINED,
        rhi_texture_layout final_layout = RHI_TEXTURE_LAYOUT_UNDEFINED);
    rdg_texture* add_texture(
        std::string_view name,
        rhi_texture_extent extent,
        rhi_format format,
        rhi_texture_flags flags,
        std::uint32_t level_count = 1,
        std::uint32_t layer_count = 1,
        rhi_sample_count samples = RHI_SAMPLE_COUNT_1);

    rdg_buffer* add_buffer(std::string_view name, rhi_buffer* buffer);
    rdg_buffer* add_buffer(std::string_view name, std::size_t size, rhi_buffer_flags flags);

    template <typename T, typename SetupFunctor, typename ExecuteFunctor>
        requires std::is_invocable_v<SetupFunctor, T&, rdg_pass&> &&
                 std::is_invocable_v<ExecuteFunctor, const T&, rdg_command&>
    void add_pass(
        std::string_view name,
        rdg_pass_type pass_type,
        SetupFunctor&& setup,
        ExecuteFunctor&& execute)
    {
        auto* pass = m_allocator->allocate_pass<rdg_lambda_pass<T>>();
        pass->set_name(name);
        pass->set_pass_type(pass_type);
        pass->set_execute(std::forward<ExecuteFunctor>(execute));
        pass->setup(setup);

        m_passes.push_back(pass);
        m_label_offset.push_back(m_labels.size());
    }

    void begin_group(std::string_view group_name);
    void end_group();

    void compile();
    void record(rhi_command* command);

    render_graph& operator=(const render_graph&) = delete;

private:
    void cull();
    void allocate_resources();
    void merge_passes();
    void build_barriers();

    static void allocate_texture(rdg_texture* texture);
    static void allocate_buffer(rdg_buffer* buffer);

    void build_texture_barriers(rdg_texture* texture);
    void build_buffer_barriers(rdg_buffer* buffer);

    void add_texture_barrier(
        const rdg_reference* prev_reference,
        const rdg_reference* curr_reference,
        std::uint32_t level,
        std::uint32_t level_count,
        std::uint32_t layer,
        std::uint32_t layer_count);
    void add_buffer_barrier(
        const rdg_reference* prev_reference,
        const rdg_reference* curr_reference,
        std::size_t offset,
        std::size_t size);

    std::vector<rdg_resource*> m_resources;
    std::vector<rdg_pass*> m_passes;

    std::vector<std::size_t> m_label_offset;
    std::vector<std::string> m_labels;

    struct batch
    {
        std::vector<rhi_texture_barrier> texture_barriers;
        std::vector<rhi_buffer_barrier> buffer_barriers;

        rhi_render_pass* render_pass;
        std::vector<rhi_attachment> attachments;

        rdg_pass* begin_pass;
        rdg_pass* end_pass;
    };
    std::vector<batch> m_batches;

    rdg_pass m_final_pass;

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
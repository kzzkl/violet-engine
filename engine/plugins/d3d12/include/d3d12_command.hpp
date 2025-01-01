#pragma once

#include "d3d12_common.hpp"
#include "d3d12_frame_resource.hpp"
#include <mutex>
#include <queue>

namespace violet::d3d12
{
class d3d12_render_command : public rhi_render_command
{
public:
    d3d12_render_command(D3D12CommandAllocator* allocator, std::wstring_view name = L"");

    virtual void begin(
        rhi_render_pipeline* pipeline,
        rhi_resource* render_target,
        rhi_resource* render_target_resolve,
        rhi_resource* depth_stencil_buffer) override;
    virtual void end(rhi_render_pipeline* pipeline) override;
    virtual void next_pass(rhi_render_pipeline* pipeline) override;

    virtual void set_scissor(const scissor_extent* extents, std::size_t size) override;

    virtual void set_parameter(std::size_t index, rhi_pipeline_parameter* parameter) override;

    virtual void set_input_assembly_state(
        rhi_resource* const* vertex_buffers,
        std::size_t vertex_buffer_count,
        rhi_resource* index_buffer,
        primitive_topology primitive_topology) override;

    virtual void draw(std::size_t vertex_offset, std::size_t vertex_end) override;
    virtual void draw_indexed(
        std::size_t index_offset,
        std::size_t index_end,
        std::size_t vertex_base) override;

    virtual void clear_render_target(rhi_resource* render_target, const float4& color)
        override;
    virtual void clear_depth_stencil(
        rhi_resource* depth_stencil,
        bool clear_depth,
        float depth,
        bool clear_stencil,
        std::uint8_t stencil) override;

    virtual void begin(compute_pipeline_interface* pipeline) override;
    virtual void end(compute_pipeline_interface* pipeline) override;
    virtual void dispatch(std::size_t x, std::size_t y, std::size_t z) override;
    virtual void set_compute_parameter(std::size_t index, rhi_pipeline_parameter* parameter)
        override;

    void allocator(D3D12CommandAllocator* allocator) noexcept;
    void reset();
    void close();

    D3D12GraphicsCommandList* get() { return m_command_list.Get(); }

private:
    d3d12_ptr<D3D12GraphicsCommandList> m_command_list;
    D3D12CommandAllocator* m_allocator;
};

class d3d12_dynamic_command
{
public:
    d3d12_dynamic_command(
        d3d12_ptr<D3D12GraphicsCommandList> command_list,
        d3d12_ptr<D3D12CommandAllocator> allocator) noexcept;

    void close();

    D3D12GraphicsCommandList* get() noexcept { return m_command_list.Get(); }

private:
    friend class d3d12_command_queue;

    d3d12_ptr<D3D12GraphicsCommandList> m_command_list;
    d3d12_ptr<D3D12CommandAllocator> m_allocator;
};

class d3d12_command_queue
{
public:
    d3d12_command_queue(std::size_t render_concurrency);

    d3d12_render_command* allocate_render_command();
    void execute_command(d3d12_render_command* command);

    d3d12_dynamic_command allocate_dynamic_command();
    void execute_command(d3d12_dynamic_command command);

    void execute_batch();
    void flush();

    void switch_frame_resources();

    D3D12CommandQueue* command_queue() const { return m_queue.Get(); }

private:
    void wait_completed(UINT64 fence);

    struct frame_resource
    {
        std::vector<d3d12_ptr<D3D12CommandAllocator>> render_allocators;
        UINT64 fence;
        std::size_t concurrency; // Number of allocators used
    };

    d3d12_frame_resource<frame_resource> m_frame_resource;

    // render command
    std::vector<std::unique_ptr<d3d12_render_command>> m_render_command;
    std::atomic<std::size_t> m_render_command_counter;

    // dynamic command
    using dynamic_allocator_pool = std::queue<std::pair<UINT64, d3d12_ptr<D3D12CommandAllocator>>>;
    using dynamic_command_pool = std::queue<d3d12_ptr<D3D12GraphicsCommandList>>;

    dynamic_allocator_pool m_dynamic_allocator_pool;
    dynamic_command_pool m_dynamic_command_pool;
    std::vector<d3d12_ptr<D3D12GraphicsCommandList>> m_dynamic_command_batch;
    std::mutex m_dynamic_lock;

    // queue
    d3d12_ptr<D3D12CommandQueue> m_queue;
    std::vector<D3D12CommandList*> m_batch;

    // fence
    UINT64 m_fence_counter;
    d3d12_ptr<D3D12Fence> m_fence;
};
} // namespace violet::d3d12
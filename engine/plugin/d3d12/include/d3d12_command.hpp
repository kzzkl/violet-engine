#pragma once

#include "d3d12_common.hpp"
#include "d3d12_frame_resource.hpp"
#include <mutex>
#include <queue>

namespace ash::graphics::d3d12
{
enum class d3d12_render_command_type
{
    PRE_RENDER,
    RENDER,
    POST_RENDER
};

class d3d12_render_command : public render_command
{
public:
    d3d12_render_command(D3D12CommandAllocator* allocator, std::wstring_view name = L"");

    virtual void set_pipeline(pipeline* pipeline) override;
    virtual void set_layout(pipeline_parameter_layout* layout) override;
    virtual void set_parameter(std::size_t index, pipeline_parameter* parameter) override;
    virtual void draw(
        resource* vertex,
        resource* index,
        primitive_topology_type primitive_topology,
        resource* target) override;

    void set_allocator(D3D12CommandAllocator* allocator) noexcept;
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
        d3d12_ptr<D3D12CommandAllocator> allocator);

    void close();

    D3D12GraphicsCommandList* get() noexcept { return m_command_list.Get(); }

private:
    friend class d3d12_command_manager;

    d3d12_ptr<D3D12GraphicsCommandList> m_command_list;
    d3d12_ptr<D3D12CommandAllocator> m_allocator;
};

class d3d12_command_manager
{
public:
    d3d12_command_manager(std::size_t render_concurrency);

    d3d12_render_command* allocate_render_command(d3d12_render_command_type type);
    void execute_command(d3d12_render_command* command);

    d3d12_dynamic_command allocate_dynamic_command();
    void execute_command(d3d12_dynamic_command command);

    void execute_batch();

    void switch_frame_resources();

    D3D12CommandQueue* get_command_queue() const { return m_queue.Get(); }

private:
    void execute_command(D3D12GraphicsCommandList* command_list);
    void wait_completed(UINT64 fence);
    void flush();

    struct frame_resource
    {
        std::vector<d3d12_ptr<D3D12CommandAllocator>> render_allocators;
        UINT64 fence;
        std::size_t concurrency; // Number of allocators used
    };

    d3d12_frame_resource<frame_resource> m_frame_resource;

    // render command
    std::unique_ptr<d3d12_render_command> m_pre_command;
    std::unique_ptr<d3d12_render_command> m_post_command;
    std::vector<std::unique_ptr<d3d12_render_command>> m_render_command;

    std::atomic<std::size_t> m_render_command_counter;

    // dynamic command
    using dynamic_allocator_pool = std::queue<std::pair<UINT64, d3d12_ptr<D3D12CommandAllocator>>>;
    using dynamic_command_pool = std::queue<d3d12_ptr<D3D12GraphicsCommandList>>;

    dynamic_allocator_pool m_dynamic_allocator_pool;
    dynamic_command_pool m_dynamic_command_pool;
    std::mutex m_dynamic_lock;

    // queue
    d3d12_ptr<D3D12CommandQueue> m_queue;
    std::vector<D3D12CommandList*> m_batch;

    // fence
    UINT64 m_fence_counter;
    d3d12_ptr<D3D12Fence> m_fence;
};
} // namespace ash::graphics::d3d12
#include "d3d12_command.hpp"
#include "d3d12_context.hpp"
#include "d3d12_pipeline.hpp"
#include "d3d12_utility.hpp"

namespace ash::graphics::d3d12
{
d3d12_render_command::d3d12_render_command(D3D12CommandAllocator* allocator, std::wstring_view name)
    : m_allocator(allocator)
{
    auto device = d3d12_context::device();
    throw_if_failed(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator,
        nullptr,
        IID_PPV_ARGS(&m_command_list)));

    m_command_list->SetName(name.data());
    m_command_list->Close();
}

void d3d12_render_command::begin(
    render_pass_interface* render_pass,
    resource_interface* render_target,
    resource_interface* render_target_resolve,
    resource_interface* depth_stencil_buffer)
{
    auto rp = static_cast<d3d12_render_pass*>(render_pass);
    d3d12_camera_info info = {
        .render_target = static_cast<d3d12_resource*>(render_target),
        .render_target_resolve = static_cast<d3d12_resource*>(render_target_resolve),
        .depth_stencil_buffer = static_cast<d3d12_resource*>(depth_stencil_buffer)};
    rp->begin(m_command_list.Get(), info);
}

void d3d12_render_command::end(render_pass_interface* render_pass)
{
    auto rp = static_cast<d3d12_render_pass*>(render_pass);
    rp->end(m_command_list.Get());
}

void d3d12_render_command::next(render_pass_interface* render_pass)
{
    auto rp = static_cast<d3d12_render_pass*>(render_pass);
    rp->next(m_command_list.Get());
}

void d3d12_render_command::parameter(std::size_t index, pipeline_parameter_interface* parameter)
{
    d3d12_pipeline_parameter* p = static_cast<d3d12_pipeline_parameter*>(parameter);
    p->sync();
    if (p->tier() == d3d12_parameter_tier_type::TIER1)
    {
        auto tier1 = p->tier1();
        if (tier1.type == D3D12_ROOT_PARAMETER_TYPE_CBV)
            m_command_list->SetGraphicsRootConstantBufferView(
                static_cast<UINT>(index),
                tier1.address);
        else if (tier1.type == D3D12_ROOT_PARAMETER_TYPE_SRV)
            m_command_list->SetGraphicsRootShaderResourceView(
                static_cast<UINT>(index),
                tier1.address);
    }
    else
    {
        auto tier2 = p->tier2();
        m_command_list->SetGraphicsRootDescriptorTable(
            static_cast<UINT>(index),
            tier2.base_gpu_handle);
    }
}

void d3d12_render_command::scissor(const scissor_rect& rect)
{
    D3D12_RECT r = {
        static_cast<LONG>(rect.min_x),
        static_cast<LONG>(rect.min_y),
        static_cast<LONG>(rect.max_x),
        static_cast<LONG>(rect.max_y)};
    m_command_list->RSSetScissorRects(1, &r);
}

void d3d12_render_command::draw(
    resource* vertex,
    resource* index,
    std::size_t index_start,
    std::size_t index_end,
    std::size_t vertex_base)
{
    auto vb = static_cast<d3d12_resource*>(vertex)->vertex_buffer();
    auto ib = static_cast<d3d12_resource*>(index)->index_buffer();

    m_command_list->IASetVertexBuffers(0, 1, &vb.view());
    m_command_list->IASetIndexBuffer(&ib.view());

    m_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    m_command_list->DrawIndexedInstanced(
        static_cast<UINT>(index_end - index_start),
        1,
        static_cast<UINT>(index_start),
        static_cast<UINT>(vertex_base),
        0);
}

void d3d12_render_command::allocator(D3D12CommandAllocator* allocator) noexcept
{
    m_allocator = allocator;
}

void d3d12_render_command::reset()
{
    throw_if_failed(m_command_list->Reset(m_allocator, nullptr));
}

void d3d12_render_command::close()
{
    throw_if_failed(m_command_list->Close());
}

d3d12_dynamic_command::d3d12_dynamic_command(
    d3d12_ptr<D3D12GraphicsCommandList> command_list,
    d3d12_ptr<D3D12CommandAllocator> allocator) noexcept
    : m_command_list(command_list),
      m_allocator(allocator)
{
}

void d3d12_dynamic_command::close()
{
    throw_if_failed(m_command_list->Close());
}

d3d12_command_queue::d3d12_command_queue(std::size_t render_concurrency) : m_fence_counter(0)
{
    auto device = d3d12_context::device();

    // Initialize frame resource.
    for (auto& resource : m_frame_resource)
    {
        resource.fence = 0;
        resource.render_allocators.resize(render_concurrency);
        for (auto& allocator : resource.render_allocators)
        {
            throw_if_failed(device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&allocator)));
        }
    }

    // Initialize rendering commands.
    auto render_allocators = m_frame_resource.get().render_allocators;
    for (std::size_t i = 0; i < render_concurrency; ++i)
    {
        auto render_command =
            std::make_unique<d3d12_render_command>(render_allocators[i].Get(), L"render command");
        m_render_command.push_back(std::move(render_command));
    }

    // Initialize dynamic commands.

    // Initialize command queue.
    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    throw_if_failed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&m_queue)));

    throw_if_failed(
        device->CreateFence(m_fence_counter, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
}

d3d12_render_command* d3d12_command_queue::allocate_render_command()
{
    std::size_t index = m_render_command_counter.fetch_add(1);
    if (index < m_render_command.size())
    {
        m_render_command[index]->reset();
        return m_render_command[index].get();
    }
    else
    {
        return nullptr;
    }
}

void d3d12_command_queue::execute_command(d3d12_render_command* command)
{
    command->close();
}

d3d12_dynamic_command d3d12_command_queue::allocate_dynamic_command()
{
    static int allocator_counter = 0;
    static int command_counter = 0;
    auto device = d3d12_context::device();

    std::lock_guard<std::mutex> lg(m_dynamic_lock);

    d3d12_ptr<D3D12CommandAllocator> allocator;
    if (!m_dynamic_allocator_pool.empty() &&
        m_dynamic_allocator_pool.front().first < m_fence->GetCompletedValue())
    {
        allocator = m_dynamic_allocator_pool.front().second;
        m_dynamic_allocator_pool.pop();
        throw_if_failed(allocator->Reset());
    }
    else
    {
        throw_if_failed(device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&allocator)));

        std::wstring name = L"dynamic allocator " + std::to_wstring(allocator_counter);
        allocator->SetName(name.c_str());
        ++allocator_counter;
    }

    d3d12_ptr<D3D12GraphicsCommandList> command_list;
    if (!m_dynamic_command_pool.empty())
    {
        command_list = m_dynamic_command_pool.front();
        m_dynamic_command_pool.pop();

        throw_if_failed(command_list->Reset(allocator.Get(), nullptr));
    }
    else
    {
        throw_if_failed(device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            allocator.Get(),
            nullptr,
            IID_PPV_ARGS(&command_list)));

        std::wstring name = L"dynamic command " + std::to_wstring(command_counter);
        allocator->SetName(name.c_str());
        ++command_counter;
    }

    return d3d12_dynamic_command(command_list, allocator);
}

void d3d12_command_queue::execute_command(d3d12_dynamic_command command)
{
    std::lock_guard<std::mutex> lg(m_dynamic_lock);

    command.close();
    m_dynamic_command_batch.push_back(command.m_command_list);
    m_dynamic_allocator_pool.push(std::make_pair(m_fence_counter, command.m_allocator));
}

void d3d12_command_queue::execute_batch()
{
    // execute dynamic command
    if (!m_dynamic_command_batch.empty())
    {
        for (auto& command : m_dynamic_command_batch)
        {
            m_batch.push_back(command.Get());
            m_dynamic_command_pool.push(command);
        }
        m_dynamic_command_batch.clear();

        m_queue->ExecuteCommandLists(static_cast<UINT>(m_batch.size()), m_batch.data());
        m_batch.clear();
    }

    // execute render command
    for (std::size_t i = 0; i < m_render_command_counter; ++i)
        m_batch.push_back(m_render_command[i]->get());

    m_queue->ExecuteCommandLists(static_cast<UINT>(m_batch.size()), m_batch.data());
    m_batch.clear();

    ++m_fence_counter;
    throw_if_failed(m_queue->Signal(m_fence.Get(), m_fence_counter));

    auto& resource = m_frame_resource.get();
    resource.fence = m_fence_counter;
    resource.concurrency = m_render_command_counter;
}

void d3d12_command_queue::flush()
{
    ++m_fence_counter;
    throw_if_failed(m_queue->Signal(m_fence.Get(), m_fence_counter));
    wait_completed(m_fence_counter);
}

void d3d12_command_queue::switch_frame_resources()
{
    auto& resource = m_frame_resource.get();

    wait_completed(resource.fence);

    auto& allocators = resource.render_allocators;
    for (std::size_t i = 0; i < resource.concurrency; ++i)
        throw_if_failed(allocators[i]->Reset());

    for (std::size_t i = 0; i < m_render_command.size(); ++i)
        m_render_command[i]->allocator(allocators[i].Get());

    m_render_command_counter = 0;
}

void d3d12_command_queue::wait_completed(UINT64 fence)
{
    if (m_fence->GetCompletedValue() < fence)
    {
        HANDLE event = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        throw_if_failed(m_fence->SetEventOnCompletion(fence, event));

        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
}
} // namespace ash::graphics::d3d12
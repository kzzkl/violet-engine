#include "d3d12_command.hpp"
#include "d3d12_context.hpp"

namespace ash::graphics::d3d12
{
d3d12_command_manager::d3d12_command_manager() : m_fence_counter(0)
{
}

bool d3d12_command_manager::initialize()
{
    auto device = d3d12_context::instance().get_device();

    D3D12_COMMAND_QUEUE_DESC queue_desc = {};
    queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

    throw_if_failed(device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(m_queue.GetAddressOf())));
    throw_if_failed(device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        IID_PPV_ARGS(m_allocator.GetAddressOf())));

    throw_if_failed(device->CreateCommandList(
        0,
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_allocator.Get(),
        nullptr,
        IID_PPV_ARGS(m_command_list.GetAddressOf())));

    throw_if_failed(device->CreateFence(
        m_fence_counter,
        D3D12_FENCE_FLAG_NONE,
        IID_PPV_ARGS(m_fence.GetAddressOf())));

    return true;
}

void d3d12_command_manager::flush()
{
    ++m_fence_counter;

    throw_if_failed(m_queue->Signal(m_fence.Get(), m_fence_counter));

    if (m_fence->GetCompletedValue() < m_fence_counter)
    {
        HANDLE event = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        throw_if_failed(m_fence->SetEventOnCompletion(m_fence_counter, event));

        WaitForSingleObject(event, INFINITE);
        CloseHandle(event);
    }
}
} // namespace ash::graphics::d3d12
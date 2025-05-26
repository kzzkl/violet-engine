#include "graphics/resources/persistent_buffer.hpp"
#include "gpu_buffer_uploader.hpp"
#include <limits>

namespace violet
{
persistent_buffer::persistent_buffer(std::size_t initial_size, rhi_buffer_flags flags)
    : m_allocator(std::numeric_limits<std::uint32_t>::max())
{
    set_buffer({
        .size = initial_size,
        .flags = flags | RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_TRANSFER_DST,
    });
}

buffer_allocation persistent_buffer::allocate(std::size_t size)
{
    buffer_allocation allocation = m_allocator.allocate(size);
    assert(allocation.offset != buffer_allocation::NO_SPACE);

    m_allocated_size = std::max(m_allocated_size, allocation.offset + size);

    return allocation;
}

void persistent_buffer::free(buffer_allocation allocation)
{
    m_allocator.free(allocation);
}

void persistent_buffer::copy(const void* data, std::size_t size, std::size_t offset)
{
    assert(size > 0);

    m_copy_queue.emplace_back(copy_command{
        .data = data,
        .size = size,
        .offset = offset,
    });
}

void persistent_buffer::upload(
    gpu_buffer_uploader* uploader,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access)
{
    if (get_rhi()->get_size() < m_allocated_size)
    {
        reserve();
    }

    rhi_buffer* buffer = get_rhi();
    for (auto& command : m_copy_queue)
    {
        uploader->upload(buffer, command.data, command.size, command.offset, stages, access);
    }

    m_copy_queue.clear();
}

void persistent_buffer::reserve()
{
    rhi_buffer* old_buffer = get_rhi();

    std::size_t new_buffer_size = old_buffer->get_size();
    while (m_allocated_size > new_buffer_size)
    {
        new_buffer_size *= 2;
    }

    auto& device = render_device::instance();

    auto new_buffer = device.create_buffer({
        .size = new_buffer_size,
        .flags = old_buffer->get_flags(),
    });

    rhi_command* command = device.allocate_command();

    rhi_buffer_region region = {
        .offset = 0,
        .size = old_buffer->get_size(),
    };
    command->copy_buffer(old_buffer, region, new_buffer.get(), region);

    rhi_buffer_barrier barrier = {
        .buffer = new_buffer.get(),
        .src_stages = RHI_PIPELINE_STAGE_TRANSFER,
        .src_access = RHI_ACCESS_TRANSFER_WRITE,
        .dst_stages =
            RHI_PIPELINE_STAGE_VERTEX | RHI_PIPELINE_STAGE_FRAGMENT | RHI_PIPELINE_STAGE_COMPUTE,
        .dst_access = RHI_ACCESS_SHADER_READ,
        .offset = 0,
        .size = old_buffer->get_size(),
    };
    command->set_pipeline_barrier(&barrier, 1, nullptr, 0);

    device.execute(command);

    set_buffer(std::move(new_buffer));
}
} // namespace violet
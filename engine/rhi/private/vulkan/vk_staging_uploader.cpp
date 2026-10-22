#include "vk_staging_uploader.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_vma.hpp"
#include <cassert>

namespace violet::vk
{
vk_staging_uploader::vk_staging_uploader(std::size_t staging_buffer_size, vk_context* context)
    : m_staging_buffer_size(staging_buffer_size),
      m_context(context)
{
    VkBufferCreateInfo staging_buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = staging_buffer_size,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    };

    VmaAllocationInfo staging_allocation_info = {};

    std::tie(m_staging_buffer, m_staging_allocation) = vk_vma::create_buffer(
        staging_buffer_info,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        m_context->get_vma_allocator(),
        &staging_allocation_info);

    m_mapped_data = staging_allocation_info.pMappedData;
}

vk_staging_uploader::~vk_staging_uploader()
{
    vk_vma::destroy_buffer(m_staging_buffer, m_staging_allocation, m_context->get_vma_allocator());
}

void vk_staging_uploader::upload(
    const void* data,
    std::size_t size,
    VkBuffer buffer,
    std::size_t offset)
{
    std::scoped_lock lock(m_mutex);

    auto* queue = m_context->get_graphics_queue();

    std::size_t pending_size = size;
    while (pending_size > 0)
    {
        auto* command = queue->allocate_command();

        std::size_t upload_size = std::min(m_staging_buffer_size, pending_size);
        std::size_t upload_offset = offset + size - pending_size;

        std::memcpy(
            m_mapped_data,
            static_cast<const std::byte*>(data) + upload_offset,
            upload_size);

        VkBufferCopy copy_region = {
            .srcOffset = 0,
            .dstOffset = upload_offset,
            .size = upload_size,
        };
        vkCmdCopyBuffer(command->get_command_buffer(), m_staging_buffer, buffer, 1, &copy_region);

        queue->execute_sync(command);

        pending_size -= upload_size;
    }
}
} // namespace violet::vk
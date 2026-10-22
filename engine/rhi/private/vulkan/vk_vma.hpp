#pragma once

#include "vk_common.hpp"
#include <cassert>

namespace violet::vk
{
class vk_vma
{
public:
    static std::pair<VkBuffer, VmaAllocation> create_buffer(
        const VkBufferCreateInfo& buffer_info,
        VmaAllocationCreateFlags flags,
        VmaAllocator allocator,
        VmaAllocationInfo* allocation_info = nullptr)
    {
        assert(buffer_info.size != 0);

        VkBuffer buffer;
        VmaAllocation allocation;

        VmaAllocationCreateInfo create_info = {
            .flags = flags,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        vk_check(vmaCreateBuffer(
            allocator,
            &buffer_info,
            &create_info,
            &buffer,
            &allocation,
            allocation_info));

        return {buffer, allocation};
    }

    static void destroy_buffer(VkBuffer buffer, VmaAllocation allocation, VmaAllocator allocator)
    {
        vmaDestroyBuffer(allocator, buffer, allocation);
    }

    static std::pair<VkImage, VmaAllocation> create_image(
        const VkImageCreateInfo& image_info,
        VmaAllocationCreateFlags flags,
        VmaAllocator allocator,
        VmaAllocationInfo* allocation_info = nullptr)
    {
        VkImage image;
        VmaAllocation allocation;

        VmaAllocationCreateInfo create_info = {
            .flags = flags,
            .usage = VMA_MEMORY_USAGE_AUTO,
        };

        vk_check(vmaCreateImage(
            allocator,
            &image_info,
            &create_info,
            &image,
            &allocation,
            allocation_info));

        return {image, allocation};
    }

    static void destroy_image(VkImage image, VmaAllocation allocation, VmaAllocator allocator)
    {
        vmaDestroyImage(allocator, image, allocation);
    }
};
} // namespace violet::vk
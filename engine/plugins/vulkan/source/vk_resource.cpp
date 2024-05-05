#include "vk_resource.hpp"
#include "vk_command.hpp"
#include "vk_image_loader.hpp"
#include "vk_util.hpp"
#include <cassert>
#include <cmath>

namespace violet::vk
{
namespace
{
void create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags buffer_usage,
    VmaMemoryUsage memory_usage,
    VkBuffer& buffer,
    VmaAllocation& allocation,
    VmaAllocator allocator)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = buffer_usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = memory_usage;

    vk_check(
        vmaCreateBuffer(allocator, &buffer_info, &allocation_info, &buffer, &allocation, nullptr));
}

void create_host_visible_buffer(
    const void* data,
    VkDeviceSize size,
    VkBufferUsageFlags buffer_usage,
    VkBuffer& buffer,
    VmaAllocation& allocation,
    VmaAllocator allocator)
{
    create_buffer(size, buffer_usage, VMA_MEMORY_USAGE_CPU_TO_GPU, buffer, allocation, allocator);

    if (data != nullptr)
    {
        void* mapping_pointer = nullptr;
        vmaMapMemory(allocator, allocation, &mapping_pointer);
        std::memcpy(mapping_pointer, data, size);
        vmaUnmapMemory(allocator, allocation);
    }
}

void create_image(
    const VkImageCreateInfo& image_info,
    VmaMemoryUsage memory_usage,
    VkImage& image,
    VmaAllocation& allocation,
    VmaAllocator allocator)
{
    VmaAllocationCreateInfo allocation_info = {};
    allocation_info.usage = memory_usage;

    vk_check(
        vmaCreateImage(allocator, &image_info, &allocation_info, &image, &allocation, nullptr));
}
} // namespace

vk_image::vk_image(vk_context* context)
    : m_context(context),
      m_image(VK_NULL_HANDLE),
      m_allocation(VK_NULL_HANDLE),
      m_image_view(VK_NULL_HANDLE),
      m_format(VK_FORMAT_UNDEFINED),
      m_extent{0, 0}
{
    m_clear_value.color = {0.0f, 0.0f, 0.0f, 1.0f};
}

vk_image::vk_image(const rhi_texture_desc& desc, vk_context* context) : vk_image(context)
{
    VkFormat format = vk_util::map_format(desc.format);

    VkImage image;
    VmaAllocation allocation;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = desc.width;
    image_info.extent.height = desc.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = vk_util::map_sample_count(desc.samples);
    image_info.flags = 0;

    if (desc.flags & RHI_TEXTURE_FLAG_RENDER_TARGET)
        image_info.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (desc.flags & RHI_TEXTURE_FLAG_DEPTH_STENCIL)
        image_info.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    if (desc.flags & RHI_TEXTURE_FLAG_TRANSFER_SRC)
        image_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    if (desc.flags & RHI_TEXTURE_FLAG_TRANSFER_DST)
        image_info.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    create_image(
        image_info,
        VMA_MEMORY_USAGE_GPU_ONLY,
        image,
        allocation,
        get_context()->get_vma_allocator());

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    if (desc.flags & RHI_TEXTURE_FLAG_DEPTH_STENCIL)
        image_view_info.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageView image_view;
    vkCreateImageView(get_context()->get_device(), &image_view_info, nullptr, &image_view);

    set_image(image, allocation);
    set_image_view(image_view);
    set_format(format);
    set_extent(VkExtent2D{desc.width, desc.height});

    if (desc.flags & RHI_TEXTURE_FLAG_DEPTH_STENCIL)
        set_clear_value(VkClearDepthStencilValue{1.0, 0});
    else
        set_clear_value(VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f});
}

vk_image::~vk_image()
{
    vmaDestroyImage(m_context->get_vma_allocator(), m_image, m_allocation);
    vkDestroyImageView(m_context->get_device(), m_image_view, nullptr);
}

rhi_format vk_image::get_format() const noexcept
{
    return vk_util::map_format(m_format);
}

void vk_image::set_image(VkImage image, VmaAllocation allocation) noexcept
{
    m_image = image;
    m_allocation = allocation;

    m_hash = vk_util::hash(m_image);
}

vk_texture::vk_texture(const char* file, const rhi_texture_desc& desc, vk_context* context)
    : vk_image(context)
{
    vk_image_loader loader;
    if (!loader.load(file))
        throw vk_exception("Failed to load image.");

    const vk_image_data& data = loader.get_mipmap(0);
    std::uint32_t mip_levels = loader.get_mipmap_count();
    if (mip_levels == 1 && desc.flags & RHI_TEXTURE_FLAG_MIPMAP)
    {
        mip_levels =
            static_cast<std::uint32_t>(std::floor(std::log2((std::max)(data.width, data.height)))) +
            1;
    }

    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    create_host_visible_buffer(
        data.pixels.data(),
        data.pixels.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_buffer,
        staging_allocation,
        get_context()->get_vma_allocator());

    VkImage image;
    VmaAllocation allocation;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = data.width;
    image_info.extent.height = data.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = 1;
    image_info.format = data.format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (mip_levels != 1)
        image_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;
    create_image(
        image_info,
        VMA_MEMORY_USAGE_GPU_ONLY,
        image,
        allocation,
        get_context()->get_vma_allocator());

    vk_command* command = get_context()->get_graphics_queue()->allocate_command();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCmdPipelineBarrier(
        command->get_command_buffer(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {data.width, data.height, 1};

    vkCmdCopyBufferToImage(
        command->get_command_buffer(),
        staging_buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    if (mip_levels != 1)
    {
        int32_t mip_width = data.width;
        int32_t mip_height = data.height;

        for (std::uint32_t i = 1; i < mip_levels; ++i)
        {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.subresourceRange.levelCount = 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                command->get_command_buffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

            VkImageBlit blit = {};
            blit.srcOffsets[0] = {0, 0, 0};
            blit.srcOffsets[1] = {mip_width, mip_height, 1};
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = {0, 0, 0};
            blit.dstOffsets[1] = {
                mip_width > 1 ? mip_width / 2 : 1,
                mip_height > 1 ? mip_height / 2 : 1,
                1};
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(
                command->get_command_buffer(),
                image,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &blit,
                VK_FILTER_LINEAR);

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                command->get_command_buffer(),
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0,
                nullptr,
                0,
                nullptr,
                1,
                &barrier);

            if (mip_width > 1)
                mip_width /= 2;
            if (mip_height > 1)
                mip_height /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mip_levels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            command->get_command_buffer(),
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    }
    else
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vkCmdPipelineBarrier(
            command->get_command_buffer(),
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0,
            nullptr,
            0,
            nullptr,
            1,
            &barrier);
    }

    get_context()->get_graphics_queue()->execute_sync(command);

    vmaDestroyBuffer(get_context()->get_vma_allocator(), staging_buffer, staging_allocation);

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = data.format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = mip_levels;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    vkCreateImageView(get_context()->get_device(), &image_view_info, nullptr, &image_view);

    set_image(image, allocation);
    set_image_view(image_view);
    set_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    set_format(data.format);
    set_extent(VkExtent2D{data.width, data.height});
}

vk_texture::~vk_texture()
{
}

vk_texture_cube::vk_texture_cube(
    const char* right,
    const char* left,
    const char* top,
    const char* bottom,
    const char* front,
    const char* back,
    const rhi_texture_desc& desc,
    vk_context* context)
    : vk_image(context)
{
    std::vector<vk_image_loader> loaders(6);
    if (!loaders[0].load(right) || !loaders[1].load(left) || !loaders[2].load(top) ||
        !loaders[3].load(bottom) || !loaders[4].load(front) || !loaders[5].load(back))
        throw vk_exception("Failed to load image.");

    std::vector<std::size_t> data_offset(6);
    std::size_t data_size = 0;
    for (std::size_t i = 0; i < 6; ++i)
    {
        if (i > 0)
            data_offset[i] = data_offset[i - 1] + loaders[i - 1].get_mipmap(0).pixels.size();

        data_size += loaders[i].get_mipmap(0).pixels.size();
    }

    std::vector<char> data(data_size);
    for (std::size_t i = 0; i < 6; ++i)
    {
        std::memcpy(
            data.data() + data_offset[i],
            loaders[i].get_mipmap(0).pixels.data(),
            loaders[i].get_mipmap(0).pixels.size());
    }

    VkBuffer staging_buffer;
    VmaAllocation staging_allocation;
    create_host_visible_buffer(
        data.data(),
        data.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_buffer,
        staging_allocation,
        get_context()->get_vma_allocator());

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = loaders[0].get_mipmap(0).width;
    image_info.extent.height = loaders[0].get_mipmap(0).height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.format = loaders[0].get_mipmap(0).format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (desc.flags & RHI_TEXTURE_FLAG_STORAGE)
        image_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    VkImage image;
    VmaAllocation allocation;
    create_image(
        image_info,
        VMA_MEMORY_USAGE_GPU_ONLY,
        image,
        allocation,
        get_context()->get_vma_allocator());

    vk_command* command = get_context()->get_graphics_queue()->allocate_command();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 6;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCmdPipelineBarrier(
        command->get_command_buffer(),
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    std::vector<VkBufferImageCopy> regions;
    for (std::uint32_t i = 0; i < 6; ++i)
    {
        VkBufferImageCopy region = {};
        region.bufferOffset = data_offset[i];
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = i;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {image_info.extent.width, image_info.extent.height, 1};

        regions.push_back(region);
    }

    vkCmdCopyBufferToImage(
        command->get_command_buffer(),
        staging_buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        static_cast<std::uint32_t>(regions.size()),
        regions.data());

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vkCmdPipelineBarrier(
        command->get_command_buffer(),
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    get_context()->get_graphics_queue()->execute_sync(command);

    vmaDestroyBuffer(get_context()->get_vma_allocator(), staging_buffer, staging_allocation);

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    image_view_info.format = image_info.format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 6;

    VkImageView image_view;
    vkCreateImageView(get_context()->get_device(), &image_view_info, nullptr, &image_view);

    set_image(image, allocation);
    set_image_view(image_view);
    set_image_layout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    set_format(image_info.format);
    set_extent(VkExtent2D{image_info.extent.width, image_info.extent.height});
}

vk_texture_cube::vk_texture_cube(const rhi_texture_desc& desc, vk_context* context)
    : vk_image(context)
{
    VkImage image;
    VmaAllocation allocation;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = desc.width;
    image_info.extent.height = desc.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 6;
    image_info.format = vk_util::map_format(desc.format);
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (desc.flags & RHI_TEXTURE_FLAG_STORAGE)
        image_info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

    create_image(
        image_info,
        VMA_MEMORY_USAGE_GPU_ONLY,
        image,
        allocation,
        get_context()->get_vma_allocator());

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    image_view_info.format = image_info.format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    vkCreateImageView(get_context()->get_device(), &image_view_info, nullptr, &image_view);
}

vk_texture_cube::~vk_texture_cube()
{
}

vk_sampler::vk_sampler(const rhi_sampler_desc& desc, vk_context* context) : m_context(context)
{
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = vk_util::map_filter(desc.mag_filter);
    sampler_info.minFilter = vk_util::map_filter(desc.min_filter);
    sampler_info.addressModeU = vk_util::map_sampler_address_mode(desc.address_mode_u);
    sampler_info.addressModeV = vk_util::map_sampler_address_mode(desc.address_mode_v);
    sampler_info.addressModeW = vk_util::map_sampler_address_mode(desc.address_mode_w);
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy =
        m_context->get_physical_device_properties().limits.maxSamplerAnisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.minLod = desc.min_mip_level;
    sampler_info.maxLod = desc.max_mip_level;

    vk_check(vkCreateSampler(m_context->get_device(), &sampler_info, nullptr, &m_sampler));
}

vk_sampler::~vk_sampler()
{
    vkDestroySampler(m_context->get_device(), m_sampler, nullptr);
}

vk_buffer::vk_buffer(vk_context* context)
    : m_context(context),
      m_buffer(VK_NULL_HANDLE),
      m_allocation(VK_NULL_HANDLE),
      m_buffer_size(0),
      m_mapping_pointer(nullptr)
{
}

vk_buffer::~vk_buffer()
{
    if (m_mapping_pointer)
        vmaUnmapMemory(get_context()->get_vma_allocator(), m_allocation);

    vmaDestroyBuffer(get_context()->get_vma_allocator(), m_buffer, m_allocation);
}

std::size_t vk_buffer::get_hash() const noexcept
{
    return vk_util::hash(m_buffer);
}

vk_vertex_buffer::vk_vertex_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(context)
{
    assert(desc.flags & RHI_BUFFER_FLAG_VERTEX);

    VkBuffer buffer;
    VmaAllocation allocation;

    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    if (desc.flags & RHI_BUFFER_FLAG_STORAGE)
        usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if (desc.flags & RHI_BUFFER_FLAG_HOST_VISIBLE)
    {
        create_host_visible_buffer(
            desc.data,
            desc.size,
            usage_flags,
            buffer,
            allocation,
            get_context()->get_vma_allocator());

        void* mapping_pointer = nullptr;
        vmaMapMemory(get_context()->get_vma_allocator(), allocation, &mapping_pointer);
        set_mapping_pointer(mapping_pointer);
    }
    else
    {
        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;
        create_host_visible_buffer(
            desc.data,
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            staging_buffer,
            staging_allocation,
            get_context()->get_vma_allocator());

        create_buffer(
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage_flags,
            VMA_MEMORY_USAGE_GPU_ONLY,
            buffer,
            allocation,
            get_context()->get_vma_allocator());

        vk_command* command = get_context()->get_graphics_queue()->allocate_command();
        VkBufferCopy copy_region = {0, 0, desc.size};
        vkCmdCopyBuffer(command->get_command_buffer(), staging_buffer, buffer, 1, &copy_region);
        get_context()->get_graphics_queue()->execute_sync(command);

        vmaDestroyBuffer(get_context()->get_vma_allocator(), staging_buffer, staging_allocation);
    }

    set_buffer(buffer, allocation, desc.size);
}

vk_index_buffer::vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(context)
{
    assert(desc.flags & RHI_BUFFER_FLAG_INDEX);

    VkBuffer buffer;
    VmaAllocation allocation;

    VkBufferUsageFlags usage_flags = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    if (desc.flags & RHI_BUFFER_FLAG_STORAGE)
        usage_flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

    if (desc.index.size == 2)
        m_index_type = VK_INDEX_TYPE_UINT16;
    else if (desc.index.size == 4)
        m_index_type = VK_INDEX_TYPE_UINT32;
    else
        throw vk_exception("Invalid index size.");

    if (desc.flags & RHI_BUFFER_FLAG_HOST_VISIBLE)
    {
        create_host_visible_buffer(
            desc.data,
            desc.size,
            usage_flags,
            buffer,
            allocation,
            get_context()->get_vma_allocator());

        void* mapping_pointer = nullptr;
        vmaMapMemory(get_context()->get_vma_allocator(), allocation, &mapping_pointer);
        set_mapping_pointer(mapping_pointer);
    }
    else
    {
        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;
        create_host_visible_buffer(
            desc.data,
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            staging_buffer,
            staging_allocation,
            get_context()->get_vma_allocator());

        create_buffer(
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage_flags,
            VMA_MEMORY_USAGE_GPU_ONLY,
            buffer,
            allocation,
            get_context()->get_vma_allocator());

        vk_command* command = get_context()->get_graphics_queue()->allocate_command();
        VkBufferCopy copy_region = {0, 0, desc.size};
        vkCmdCopyBuffer(command->get_command_buffer(), staging_buffer, buffer, 1, &copy_region);
        get_context()->get_graphics_queue()->execute_sync(command);

        vmaDestroyBuffer(get_context()->get_vma_allocator(), staging_buffer, staging_allocation);
    }

    set_buffer(buffer, allocation, desc.size);
}

vk_uniform_buffer::vk_uniform_buffer(void* data, std::size_t size, vk_context* context)
    : vk_buffer(context)
{
    VkBuffer buffer;
    VmaAllocation allocation;

    create_host_visible_buffer(
        data,
        size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        buffer,
        allocation,
        get_context()->get_vma_allocator());

    void* mapping_pointer = nullptr;
    vmaMapMemory(get_context()->get_vma_allocator(), allocation, &mapping_pointer);

    set_buffer(buffer, allocation, size);
    set_mapping_pointer(mapping_pointer);
}

vk_storage_buffer::vk_storage_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(context)
{
    VkBuffer buffer;
    VmaAllocation allocation;

    if (desc.flags & RHI_BUFFER_FLAG_HOST_VISIBLE)
    {
        create_host_visible_buffer(
            desc.data,
            desc.size,
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            buffer,
            allocation,
            get_context()->get_vma_allocator());

        void* mapping_pointer = nullptr;
        vmaMapMemory(get_context()->get_vma_allocator(), allocation, &mapping_pointer);
        set_mapping_pointer(mapping_pointer);
    }
    else
    {
        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;
        create_host_visible_buffer(
            desc.data,
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            staging_buffer,
            staging_allocation,
            get_context()->get_vma_allocator());

        create_buffer(
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY,
            buffer,
            allocation,
            get_context()->get_vma_allocator());

        vk_command* command = get_context()->get_graphics_queue()->allocate_command();
        VkBufferCopy copy_region = {0, 0, desc.size};
        vkCmdCopyBuffer(command->get_command_buffer(), staging_buffer, buffer, 1, &copy_region);
        get_context()->get_graphics_queue()->execute_sync(command);

        vmaDestroyBuffer(get_context()->get_vma_allocator(), staging_buffer, staging_allocation);
    }

    set_buffer(buffer, allocation, desc.size);
}
} // namespace violet::vk
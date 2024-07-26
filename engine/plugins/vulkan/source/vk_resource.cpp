#include "vk_resource.hpp"
#include "vk_command.hpp"
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

void copy_buffer_to_image(
    VkCommandBuffer command,
    VkImage image,
    VkExtent3D extent,
    std::uint32_t mip_level,
    std::uint32_t layer,
    VkBuffer buffer)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = mip_level;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = layer;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(
        command,
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
    region.imageSubresource.mipLevel = mip_level;
    region.imageSubresource.baseArrayLayer = layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = extent;

    vkCmdCopyBufferToImage(
        command,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    vkCmdPipelineBarrier(
        command,
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
} // namespace

vk_texture::vk_texture(const rhi_texture_desc& desc, vk_context* context)
    : m_flags(desc.flags),
      m_context(context)
{
    bool is_cube = desc.flags & RHI_TEXTURE_CUBE;
    m_level_count = desc.level_count;
    m_layer_count = desc.layer_count;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.arrayLayers = m_layer_count;
    image_info.extent = {desc.extent.width, desc.extent.height, 1};
    image_info.format = vk_util::map_format(desc.format);
    image_info.mipLevels = m_level_count;
    image_info.samples = vk_util::map_sample_count(desc.samples);
    image_info.usage |= (desc.flags & RHI_TEXTURE_SHADER_RESOURCE) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
    image_info.usage |= (desc.flags & RHI_TEXTURE_RENDER_TARGET) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
    image_info.usage |= (desc.flags & RHI_TEXTURE_DEPTH_STENCIL) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0;
    image_info.usage |= (desc.flags & RHI_TEXTURE_TRANSFER_SRC) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
    image_info.usage |= (desc.flags & RHI_TEXTURE_TRANSFER_DST) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
    image_info.flags = is_cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    create_image(
        image_info,
        VMA_MEMORY_USAGE_GPU_ONLY,
        m_image,
        m_allocation,
        m_context->get_vma_allocator());

    m_format = vk_util::map_format(image_info.format);
    m_samples = desc.samples;
    m_extent = {image_info.extent.width, image_info.extent.height};
    m_hash = hash::city_hash_64(&m_image, sizeof(VkImage));

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = m_image;
    image_view_info.viewType =
        (desc.flags & RHI_TEXTURE_CUBE) ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = vk_util::map_format(m_format);
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = m_level_count;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = m_layer_count;

    if (desc.flags & RHI_TEXTURE_DEPTH_STENCIL)
        image_view_info.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCreateImageView(m_context->get_device(), &image_view_info, nullptr, &m_image_view);

    if (desc.flags & RHI_TEXTURE_DEPTH_STENCIL)
        m_clear_value.depthStencil = VkClearDepthStencilValue{1.0, 0};
    else
        m_clear_value.color = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
}

vk_texture::~vk_texture()
{
    vmaDestroyImage(m_context->get_vma_allocator(), m_image, m_allocation);
    vkDestroyImageView(m_context->get_device(), m_image_view, nullptr);
}

vk_texture_view::vk_texture_view(const rhi_texture_view_desc& desc, vk_context* context)
    : m_context(context)
{
    assert(static_cast<vk_image*>(desc.texture)->is_texture_view() == false);

    m_texture = static_cast<vk_texture*>(desc.texture);
    m_level = desc.level;

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = m_texture->get_image();
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = vk_util::map_format(m_texture->get_format());
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.baseMipLevel = desc.level;
    image_view_info.subresourceRange.levelCount = desc.layer_count;
    image_view_info.subresourceRange.baseArrayLayer = desc.layer;
    image_view_info.subresourceRange.layerCount = desc.layer_count;

    if (m_texture->is_depth())
        image_view_info.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    vkCreateImageView(m_context->get_device(), &image_view_info, nullptr, &m_image_view);
}

vk_texture_view::~vk_texture_view()
{
    vkDestroyImageView(m_context->get_device(), m_image_view, nullptr);
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
    sampler_info.minLod = desc.min_level;
    sampler_info.maxLod = desc.max_level;

    vk_check(vkCreateSampler(m_context->get_device(), &sampler_info, nullptr, &m_sampler));
}

vk_sampler::~vk_sampler()
{
    vkDestroySampler(m_context->get_device(), m_sampler, nullptr);
}

vk_buffer::vk_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : m_context(context),
      m_buffer_size(desc.size),
      m_mapping_pointer(nullptr)
{
    VkBufferUsageFlags usage_flags = 0;
    usage_flags |= (desc.flags & RHI_BUFFER_VERTEX) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
    usage_flags |= (desc.flags & RHI_BUFFER_INDEX) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
    usage_flags |= (desc.flags & RHI_BUFFER_UNIFORM) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
    usage_flags |= (desc.flags & RHI_BUFFER_STORAGE) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
    usage_flags |= (desc.flags & RHI_BUFFER_TRANSFER_SRC) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;
    usage_flags |= (desc.flags & RHI_BUFFER_TRANSFER_DST) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;

    if (desc.flags & RHI_BUFFER_HOST_VISIBLE)
    {
        create_host_visible_buffer(
            desc.data,
            desc.size,
            usage_flags,
            m_buffer,
            m_allocation,
            m_context->get_vma_allocator());

        vmaMapMemory(m_context->get_vma_allocator(), m_allocation, &m_mapping_pointer);
    }
    else if (desc.data != nullptr)
    {
        VkBuffer staging_buffer;
        VmaAllocation staging_allocation;
        create_host_visible_buffer(
            desc.data,
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            staging_buffer,
            staging_allocation,
            m_context->get_vma_allocator());

        create_buffer(
            desc.size,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage_flags,
            VMA_MEMORY_USAGE_GPU_ONLY,
            m_buffer,
            m_allocation,
            m_context->get_vma_allocator());

        vk_command* command = m_context->get_graphics_queue()->allocate_command();
        VkBufferCopy copy_region = {0, 0, desc.size};
        vkCmdCopyBuffer(command->get_command_buffer(), staging_buffer, m_buffer, 1, &copy_region);
        m_context->get_graphics_queue()->execute_sync(command);

        vmaDestroyBuffer(m_context->get_vma_allocator(), staging_buffer, staging_allocation);
    }
    else
    {
        create_buffer(
            desc.size,
            usage_flags,
            VMA_MEMORY_USAGE_GPU_ONLY,
            m_buffer,
            m_allocation,
            m_context->get_vma_allocator());
    }
}

vk_buffer::~vk_buffer()
{
    if (m_mapping_pointer)
        vmaUnmapMemory(m_context->get_vma_allocator(), m_allocation);

    vmaDestroyBuffer(m_context->get_vma_allocator(), m_buffer, m_allocation);
}

std::uint64_t vk_buffer::get_hash() const noexcept
{
    return hash::city_hash_64(&m_buffer, sizeof(VkBuffer));
}

vk_index_buffer::vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(desc, context)
{
    if (desc.index.size == 2)
        m_index_type = VK_INDEX_TYPE_UINT16;
    else if (desc.index.size == 4)
        m_index_type = VK_INDEX_TYPE_UINT32;
    else
        throw vk_exception("Invalid index size.");
}
} // namespace violet::vk
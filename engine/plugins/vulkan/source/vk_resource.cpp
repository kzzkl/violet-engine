#include "vk_resource.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_util.hpp"
#include <cassert>

namespace violet::vk
{
namespace
{
std::pair<VkBuffer, VmaAllocation> create_buffer(
    const VkBufferCreateInfo& buffer_info,
    VmaAllocationCreateFlags flags,
    VmaAllocator allocator,
    VmaAllocationInfo* allocation_info = nullptr)
{
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

std::pair<VkImage, VmaAllocation> create_image(
    const VkImageCreateInfo& image_info,
    VmaAllocationCreateFlags flags,
    VmaAllocator allocator,
    VmaAllocationInfo* allocation_info = nullptr)
{
    VkImage image;
    VmaAllocation allocation;

    VmaAllocationCreateInfo create_info = {};
    create_info.usage = VMA_MEMORY_USAGE_AUTO;
    create_info.flags = flags;

    vk_check(
        vmaCreateImage(allocator, &image_info, &create_info, &image, &allocation, allocation_info));

    return {image, allocation};
}
} // namespace

vk_image::vk_image() {}

vk_texture::vk_texture(const rhi_texture_desc& desc, vk_context* context)
    : m_context(context)
{
    assert(desc.level_count > 0 && desc.layer_count > 0);

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
    image_info.usage = vk_util::map_image_usage_flags(desc.flags);
    image_info.flags = is_cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

    std::tie(m_image, m_allocation) = create_image(
        image_info,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        m_context->get_vma_allocator());

    m_format = vk_util::map_format(image_info.format);
    m_samples = desc.samples;
    m_extent = {image_info.extent.width, image_info.extent.height};

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = m_image;
    image_view_info.viewType = is_cube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
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
    {
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        m_aspect_mask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    }
    else
    {
        image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        m_aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    vkCreateImageView(m_context->get_device(), &image_view_info, nullptr, &m_image_view);

    if (desc.flags & RHI_TEXTURE_DEPTH_STENCIL)
    {
        m_clear_value.depthStencil = VkClearDepthStencilValue{0.0, 0};
    }
    else
    {
        m_clear_value.color = VkClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
    }
}

vk_texture::~vk_texture()
{
    vmaDestroyImage(m_context->get_vma_allocator(), m_image, m_allocation);
    vkDestroyImageView(m_context->get_device(), m_image_view, nullptr);
}

vk_texture_view::vk_texture_view(const rhi_texture_view_desc& desc, vk_context* context)
    : m_level(desc.level),
      m_level_count(desc.level_count),
      m_layer(desc.layer),
      m_layer_count(desc.layer_count),
      m_context(context)
{
    m_texture = static_cast<vk_texture*>(desc.texture);

    VkImageViewCreateInfo image_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_texture->get_image(),
        .format = vk_util::map_format(m_texture->get_format()),
        .subresourceRange =
            {
                .aspectMask = m_texture->get_aspect_mask(),
                .baseMipLevel = m_level,
                .levelCount = m_level_count,
                .baseArrayLayer = m_layer,
                .layerCount = m_layer_count,
            },
    };

    switch (desc.type)
    {
    case RHI_TEXTURE_VIEW_2D: {
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        break;
    }
    case RHI_TEXTURE_VIEW_2D_ARRAY: {
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        break;
    }
    case RHI_TEXTURE_VIEW_CUBE: {
        image_view_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        break;
    }
    default:
        break;
    }

    vkCreateImageView(m_context->get_device(), &image_view_info, nullptr, &m_image_view);
}

vk_texture_view::~vk_texture_view()
{
    vkDestroyImageView(m_context->get_device(), m_image_view, nullptr);
}

vk_sampler::vk_sampler(const rhi_sampler_desc& desc, vk_context* context)
    : m_context(context)
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
      m_flags(desc.flags),
      m_mapping_pointer(nullptr)
{
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = m_buffer_size,
        .usage = vk_util::map_buffer_usage_flags(desc.flags),
    };

    if (desc.flags & RHI_BUFFER_HOST_VISIBLE)
    {
        VmaAllocationInfo allocation_info = {};

        std::tie(m_buffer, m_allocation) = create_buffer(
            buffer_info,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT,
            m_context->get_vma_allocator(),
            &allocation_info);

        assert(allocation_info.pMappedData);

        if (desc.data != nullptr)
        {
            std::memcpy(allocation_info.pMappedData, desc.data, m_buffer_size);
        }

        m_mapping_pointer = allocation_info.pMappedData;
    }
    else
    {
        if (desc.data != nullptr)
        {
            VkBufferCreateInfo staging_buffer_info = {};
            staging_buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            staging_buffer_info.size = m_buffer_size;
            staging_buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

            VmaAllocationInfo staging_allocation_info = {};
            auto [staging_buffer, staging_allocation] = create_buffer(
                staging_buffer_info,
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                    VMA_ALLOCATION_CREATE_MAPPED_BIT,
                m_context->get_vma_allocator(),
                &staging_allocation_info);

            std::memcpy(staging_allocation_info.pMappedData, desc.data, m_buffer_size);

            buffer_info.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            std::tie(m_buffer, m_allocation) = create_buffer(
                buffer_info,
                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                m_context->get_vma_allocator());

            vk_command* command = m_context->get_graphics_queue()->allocate_command();
            VkBufferCopy copy_region = {0, 0, desc.size};
            vkCmdCopyBuffer(
                command->get_command_buffer(),
                staging_buffer,
                m_buffer,
                1,
                &copy_region);
            m_context->get_graphics_queue()->execute_sync(command);

            vmaDestroyBuffer(m_context->get_vma_allocator(), staging_buffer, staging_allocation);
        }
        else
        {
            std::tie(m_buffer, m_allocation) = create_buffer(
                buffer_info,
                VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
                m_context->get_vma_allocator());
        }
    }
}

vk_buffer::~vk_buffer()
{
    vmaDestroyBuffer(m_context->get_vma_allocator(), m_buffer, m_allocation);
}

vk_index_buffer::vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(desc, context)
{
    if (desc.index.size == 2)
    {
        m_index_type = VK_INDEX_TYPE_UINT16;
    }
    else if (desc.index.size == 4)
    {
        m_index_type = VK_INDEX_TYPE_UINT32;
    }
    else
    {
        throw std::runtime_error("Invalid index size.");
    }
}

vk_texel_buffer::vk_texel_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : vk_buffer(desc, context)
{
    VkBufferViewCreateInfo buffer_view_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
        .flags = 0,
        .buffer = get_buffer(),
        .format = vk_util::map_format(desc.texel.format),
        .offset = 0,
        .range = desc.size,
    };

    vkCreateBufferView(get_context()->get_device(), &buffer_view_info, nullptr, &m_buffer_view);
}

vk_texel_buffer::~vk_texel_buffer()
{
    vkDestroyBufferView(get_context()->get_device(), m_buffer_view, nullptr);
}
} // namespace violet::vk
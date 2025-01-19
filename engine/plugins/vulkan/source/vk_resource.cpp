#include "vk_resource.hpp"
#include "vk_bindless.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_framebuffer.hpp"
#include "vk_utils.hpp"
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

vk_texture_descriptor::vk_texture_descriptor(vk_texture* texture, VkImageView image_view)
    : m_texture(texture),
      m_image_view(image_view)
{
}

vk_texture_srv::vk_texture_srv(vk_texture* texture, VkImageView image_view, vk_context* context)
    : vk_texture_descriptor(texture, image_view),
      m_context(context)
{
    m_bindless = context->get_bindless_manager()->allocate_resource(this);
}

vk_texture_srv::~vk_texture_srv()
{
    m_context->get_bindless_manager()->free_resource(m_bindless);
}

void vk_texture_srv::write(
    VkDescriptorSet descriptor_set,
    std::uint32_t binding,
    std::uint32_t array_element) const
{
    VkDescriptorImageInfo info = {
        .imageView = get_image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = binding,
        .dstArrayElement = array_element,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &info,
    };

    vkUpdateDescriptorSets(m_context->get_device(), 1, &write, 0, nullptr);
}

vk_texture_uav::vk_texture_uav(vk_texture* texture, VkImageView image_view, vk_context* context)
    : vk_texture_descriptor(texture, image_view),
      m_context(context)
{
    m_bindless = m_context->get_bindless_manager()->allocate_resource(this);
}

vk_texture_uav::~vk_texture_uav()
{
    m_context->get_bindless_manager()->free_resource(m_bindless);
}

void vk_texture_uav::write(
    VkDescriptorSet descriptor_set,
    std::uint32_t binding,
    std::uint32_t array_element) const
{
    VkDescriptorImageInfo info = {
        .imageView = get_image_view(),
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL,
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = binding,
        .dstArrayElement = array_element,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        .pImageInfo = &info,
    };

    vkUpdateDescriptorSets(m_context->get_device(), 1, &write, 0, nullptr);
}

vk_texture_rtv::vk_texture_rtv(vk_texture* texture, VkImageView image_view, vk_context* context)
    : vk_texture_descriptor(texture, image_view),
      m_context(context)
{
}

vk_texture_rtv::~vk_texture_rtv()
{
    m_context->get_framebuffer_manager()->notify_texture_deleted(get_image_view());
}

vk_texture_dsv::vk_texture_dsv(vk_texture* texture, VkImageView image_view, vk_context* context)
    : vk_texture_descriptor(texture, image_view),
      m_context(context)
{
}

vk_texture_dsv::~vk_texture_dsv()
{
    m_context->get_framebuffer_manager()->notify_texture_deleted(get_image_view());
}

vk_texture::vk_texture(const rhi_texture_desc& desc, vk_context* context)
    : m_format(desc.format),
      m_samples(desc.samples),
      m_extent(desc.extent),
      m_level_count(desc.level_count),
      m_layer_count(desc.layer_count),
      m_flags(desc.flags),
      m_context(context)
{
    assert(desc.level_count > 0 && desc.layer_count > 0);

    VkImageCreateInfo image_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = desc.flags & RHI_TEXTURE_CUBE ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0u,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = vk_utils::map_format(desc.format),
        .extent = {desc.extent.width, desc.extent.height, 1},
        .mipLevels = m_level_count,
        .arrayLayers = m_layer_count,
        .samples = vk_utils::map_sample_count(desc.samples),
        .usage = vk_utils::map_image_usage_flags(desc.flags),
    };

    std::tie(m_image, m_allocation) = create_image(image_info, 0, m_context->get_vma_allocator());

    if (desc.layout != RHI_TEXTURE_LAYOUT_UNDEFINED)
    {
        rhi_texture_barrier barrier = {
            .texture = this,
            .src_stages = RHI_PIPELINE_STAGE_BEGIN,
            .src_access = 0,
            .src_layout = RHI_TEXTURE_LAYOUT_UNDEFINED,
            .dst_stages = RHI_PIPELINE_STAGE_COMPUTE | RHI_PIPELINE_STAGE_VERTEX |
                          RHI_PIPELINE_STAGE_FRAGMENT,
            .dst_access = RHI_ACCESS_SHADER_READ,
            .dst_layout = desc.layout,
            .level = 0,
            .level_count = m_level_count,
            .layer = 0,
            .layer_count = m_layer_count,
        };

        auto* command = m_context->get_graphics_queue()->allocate_command();
        command->set_pipeline_barrier(nullptr, 0, &barrier, 1);

        // TODO: use async version?
        m_context->get_graphics_queue()->execute_sync(command);
    }
}

vk_texture::vk_texture(
    VkImage image,
    VkFormat format,
    VkExtent2D extent,
    rhi_texture_flags flags,
    vk_context* context)
    : m_image(image),
      m_format(vk_utils::map_format(format)),
      m_extent{extent.width, extent.height},
      m_level_count(1),
      m_layer_count(1),
      m_flags(flags),
      m_context(context)
{
}

vk_texture::~vk_texture()
{
    for (auto& [key, view] : m_views)
    {
        view.srv = nullptr;
        view.uav = nullptr;
        view.rtv = nullptr;
        view.dsv = nullptr;

        vkDestroyImageView(m_context->get_device(), view.image_view, nullptr);
    }

    if (m_allocation != VK_NULL_HANDLE)
    {
        vmaDestroyImage(m_context->get_vma_allocator(), m_image, m_allocation);
    }
}

rhi_texture_srv* vk_texture::get_srv(
    rhi_texture_dimension dimension,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    auto& view = get_or_create_view(dimension, level, level_count, layer, layer_count);

    if (view.srv == nullptr)
    {
        view.srv = std::make_unique<vk_texture_srv>(this, view.image_view, m_context);
    }

    return view.srv.get();
}

rhi_texture_uav* vk_texture::get_uav(
    rhi_texture_dimension dimension,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    auto& view = get_or_create_view(dimension, level, level_count, layer, layer_count);

    if (view.uav == nullptr)
    {
        view.uav = std::make_unique<vk_texture_uav>(this, view.image_view, m_context);
    }

    return view.uav.get();
}

rhi_texture_rtv* vk_texture::get_rtv(
    rhi_texture_dimension dimension,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    auto& view = get_or_create_view(dimension, level, level_count, layer, layer_count);

    if (view.rtv == nullptr)
    {
        view.rtv = std::make_unique<vk_texture_rtv>(this, view.image_view, m_context);
    }

    return view.rtv.get();
}

rhi_texture_dsv* vk_texture::get_dsv(
    rhi_texture_dimension dimension,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    auto& view = get_or_create_view(dimension, level, level_count, layer, layer_count);

    if (view.dsv == nullptr)
    {
        view.dsv = std::make_unique<vk_texture_dsv>(this, view.image_view, m_context);
    }

    return view.dsv.get();
}

vk_texture::view& vk_texture::get_or_create_view(
    rhi_texture_dimension dimension,
    std::uint32_t level,
    std::uint32_t level_count,
    std::uint32_t layer,
    std::uint32_t layer_count)
{
    level_count = level_count == 0 ? m_level_count : level_count;
    layer_count = layer_count == 0 ? m_layer_count : layer_count;

    view_key key = {
        .dimension = dimension,
        .level = level,
        .level_count = level_count,
        .layer = layer,
        .layer_count = layer_count,
    };

    auto iter = m_views.find(key);
    if (iter != m_views.end())
    {
        return iter->second;
    }

    VkImageViewType view_type;
    switch (dimension)
    {
    case RHI_TEXTURE_DIMENSION_2D:
        view_type = VK_IMAGE_VIEW_TYPE_2D;
        break;
    case RHI_TEXTURE_DIMENSION_2D_ARRAY:
        view_type = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        break;
    case RHI_TEXTURE_DIMENSION_CUBE:
        view_type = VK_IMAGE_VIEW_TYPE_CUBE;
        break;
    default:
        view_type = VK_IMAGE_VIEW_TYPE_2D;
        break;
    }

    VkImageAspectFlags aspect =
        m_flags & RHI_TEXTURE_DEPTH_STENCIL ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageViewCreateInfo image_view_info = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = m_image,
        .viewType = view_type,
        .format = vk_utils::map_format(m_format),
        .subresourceRange =
            {
                .aspectMask = aspect,
                .baseMipLevel = level,
                .levelCount = level_count,
                .baseArrayLayer = layer,
                .layerCount = layer_count,
            },
    };

    vk_check(vkCreateImageView(
        m_context->get_device(),
        &image_view_info,
        nullptr,
        &m_views[key].image_view));

    return m_views[key];
}

vk_buffer_descriptor::vk_buffer_descriptor(
    vk_buffer* buffer,
    std::size_t offset,
    std::size_t size,
    VkBufferView buffer_view)
    : m_buffer(buffer),
      m_offset(offset),
      m_size(size == 0 ? m_buffer->get_buffer_size() : size),
      m_buffer_view(buffer_view)
{
}

vk_buffer_srv::vk_buffer_srv(
    vk_buffer* buffer,
    std::size_t offset,
    std::size_t size,
    VkBufferView buffer_view,
    vk_context* context)
    : vk_buffer_descriptor(buffer, offset, size, buffer_view),
      m_context(context)
{
    m_bindless = m_context->get_bindless_manager()->allocate_resource(this);
}

vk_buffer_srv::~vk_buffer_srv()
{
    m_context->get_bindless_manager()->free_resource(m_bindless);
}

void vk_buffer_srv::write(
    VkDescriptorSet descriptor_set,
    std::uint32_t binding,
    std::uint32_t array_element) const
{
    auto* buffer = get_buffer();

    VkDescriptorBufferInfo info = {
        .buffer = buffer->get_buffer(),
        .offset = get_offset(),
        .range = get_size(),
    };

    rhi_buffer_flags flags = buffer->get_flags();

    VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (flags & RHI_BUFFER_UNIFORM)
    {
        descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    }
    else if (flags & RHI_BUFFER_UNIFORM_TEXEL)
    {
        descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    }
    else if (flags & RHI_BUFFER_STORAGE)
    {
        descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (flags & RHI_BUFFER_STORAGE_TEXEL)
    {
        descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    }
    else
    {
        throw std::runtime_error("invalid buffer type.");
    }

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = binding,
        .dstArrayElement = array_element,
        .descriptorCount = 1,
        .descriptorType = descriptor_type,
        .pBufferInfo = &info,
    };

    VkBufferView buffer_view = get_buffer_view();
    if (flags & RHI_BUFFER_UNIFORM_TEXEL || flags & RHI_BUFFER_STORAGE_TEXEL)
    {
        write.pTexelBufferView = &buffer_view;
    }

    vkUpdateDescriptorSets(m_context->get_device(), 1, &write, 0, nullptr);
}

vk_buffer_uav::vk_buffer_uav(
    vk_buffer* buffer,
    std::size_t offset,
    std::size_t size,
    VkBufferView buffer_view,
    vk_context* context)
    : vk_buffer_descriptor(buffer, offset, size, buffer_view),
      m_context(context)
{
    m_bindless = m_context->get_bindless_manager()->allocate_resource(this);
}

vk_buffer_uav::~vk_buffer_uav()
{
    m_context->get_bindless_manager()->free_resource(m_bindless);
}

void vk_buffer_uav::write(
    VkDescriptorSet descriptor_set,
    std::uint32_t binding,
    std::uint32_t array_element) const
{
    auto* buffer = get_buffer();

    VkDescriptorBufferInfo info = {
        .buffer = buffer->get_buffer(),
        .offset = 0,
        .range = buffer->get_buffer_size(),
    };

    rhi_buffer_flags flags = buffer->get_flags();

    VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    if (flags & RHI_BUFFER_STORAGE)
    {
        descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    else if (flags & RHI_BUFFER_STORAGE_TEXEL)
    {
        descriptor_type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    }
    else
    {
        throw std::runtime_error("invalid buffer type.");
    }

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = binding,
        .dstArrayElement = array_element,
        .descriptorCount = 1,
        .descriptorType = descriptor_type,
        .pBufferInfo = &info,
    };

    VkBufferView buffer_view = get_buffer_view();
    if (flags & RHI_BUFFER_STORAGE_TEXEL)
    {
        write.pTexelBufferView = &buffer_view;
    }

    vkUpdateDescriptorSets(m_context->get_device(), 1, &write, 0, nullptr);
}

vk_buffer::vk_buffer(const rhi_buffer_desc& desc, vk_context* context)
    : m_context(context),
      m_buffer_size(desc.size),
      m_flags(desc.flags),
      m_mapping_pointer(nullptr)
{
    assert(m_buffer_size > 0);

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = m_buffer_size,
        .usage = vk_utils::map_buffer_usage_flags(desc.flags),
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
            std::tie(m_buffer, m_allocation) =
                create_buffer(buffer_info, 0, m_context->get_vma_allocator());

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
            std::tie(m_buffer, m_allocation) =
                create_buffer(buffer_info, 0, m_context->get_vma_allocator());
        }
    }
}

vk_buffer::~vk_buffer()
{
    for (auto& [key, view] : m_views)
    {
        if (view.buffer_view != VK_NULL_HANDLE)
        {
            vkDestroyBufferView(m_context->get_device(), view.buffer_view, nullptr);
        }
    }

    vmaDestroyBuffer(m_context->get_vma_allocator(), m_buffer, m_allocation);
}

rhi_buffer_srv* vk_buffer::get_srv(std::size_t offset, std::size_t size, rhi_format format)
{
    auto& view = get_or_create_view(offset, size, format);

    if (view.srv == nullptr)
    {
        view.srv = std::make_unique<vk_buffer_srv>(this, offset, size, view.buffer_view, m_context);
    }

    return view.srv.get();
}

rhi_buffer_uav* vk_buffer::get_uav(std::size_t offset, std::size_t size, rhi_format format)
{
    assert(m_flags & RHI_BUFFER_STORAGE || m_flags & RHI_BUFFER_STORAGE_TEXEL);

    auto& view = get_or_create_view(offset, size, format);

    if (view.uav == nullptr)
    {
        view.uav = std::make_unique<vk_buffer_uav>(this, offset, size, view.buffer_view, m_context);
    }

    return view.uav.get();
}

vk_buffer::view& vk_buffer::get_or_create_view(
    std::size_t offset,
    std::size_t size,
    rhi_format format)
{
    size = size == 0 ? m_buffer_size : size;

    view_key key = {
        .offset = offset,
        .size = size,
    };

    auto iter = m_views.find(key);
    if (iter != m_views.end())
    {
        return iter->second;
    }

    if (m_flags & RHI_BUFFER_UNIFORM_TEXEL || m_flags & RHI_BUFFER_STORAGE_TEXEL)
    {
        VkBufferViewCreateInfo buffer_view_info = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
            .flags = 0,
            .buffer = get_buffer(),
            .format = vk_utils::map_format(format),
            .offset = offset,
            .range = size,
        };

        vk_check(vkCreateBufferView(
            m_context->get_device(),
            &buffer_view_info,
            nullptr,
            &m_views[key].buffer_view));
    }

    return m_views[key];
}

vk_sampler::vk_sampler(const rhi_sampler_desc& desc, vk_context* context)
    : m_context(context)
{
    VkSamplerCreateInfo sampler_info = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = vk_utils::map_filter(desc.mag_filter),
        .minFilter = vk_utils::map_filter(desc.min_filter),
        .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .addressModeU = vk_utils::map_sampler_address_mode(desc.address_mode_u),
        .addressModeV = vk_utils::map_sampler_address_mode(desc.address_mode_v),
        .addressModeW = vk_utils::map_sampler_address_mode(desc.address_mode_w),
        .anisotropyEnable = VK_TRUE,
        .maxAnisotropy = m_context->get_physical_device_properties().limits.maxSamplerAnisotropy,
        .compareEnable = VK_FALSE,
        .minLod = desc.min_level,
        .maxLod = desc.max_level < 0.0f ? VK_LOD_CLAMP_NONE : desc.max_level,
    };

    vk_check(vkCreateSampler(m_context->get_device(), &sampler_info, nullptr, &m_sampler));

    m_bindless = m_context->get_bindless_manager()->allocate_sampler(this);
}

vk_sampler::~vk_sampler()
{
    m_context->get_bindless_manager()->free_sampler(m_bindless);

    vkDestroySampler(m_context->get_device(), m_sampler, nullptr);
}

void vk_sampler::write(
    VkDescriptorSet descriptor_set,
    std::uint32_t binding,
    std::uint32_t array_element) const
{
    VkDescriptorImageInfo info = {
        .sampler = m_sampler,
    };

    VkWriteDescriptorSet write = {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = descriptor_set,
        .dstBinding = binding,
        .dstArrayElement = array_element,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER,
        .pImageInfo = &info,
    };

    vkUpdateDescriptorSets(m_context->get_device(), 1, &write, 0, nullptr);
}
} // namespace violet::vk
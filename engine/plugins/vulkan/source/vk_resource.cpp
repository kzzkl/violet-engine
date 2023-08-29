#include "vk_resource.hpp"
#include "vk_command.hpp"
#include "vk_image_loader.hpp"
#include "vk_rhi.hpp"
#include "vk_util.hpp"

namespace violet::vk
{
namespace
{
std::uint32_t find_memory_type(
    std::uint32_t type_filter,
    VkMemoryPropertyFlags properties,
    VkPhysicalDevice physical_device)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &memory_properties);

    for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw vk_exception("Failed to find suitable memory type!");
}
} // namespace

vk_resource::vk_resource(vk_rhi* rhi) : m_rhi(rhi)
{
}

void vk_resource::create_device_local_buffer(
    const void* data,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    assert(data != nullptr);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    create_host_visible_buffer(
        data,
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_buffer,
        staging_memory);

    create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        buffer,
        memory);

    copy_buffer(staging_buffer, buffer, size);
    destroy_buffer(staging_buffer, staging_memory);
}

void vk_resource::create_host_visible_buffer(
    const void* data,
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    create_buffer(
        size,
        usage,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        buffer,
        memory);

    if (data != nullptr)
    {
        void* mapping_pointer = nullptr;
        vkMapMemory(m_rhi->get_device(), memory, 0, size, 0, &mapping_pointer);
        std::memcpy(mapping_pointer, data, size);
        vkUnmapMemory(m_rhi->get_device(), memory);
    }
}

void vk_resource::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    VkDevice device = m_rhi->get_device();

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throw_if_failed(vkCreateBuffer(device, &buffer_info, nullptr, &buffer));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, buffer, &requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex =
        find_memory_type(requirements.memoryTypeBits, properties, m_rhi->get_physical_device());

    throw_if_failed(vkAllocateMemory(device, &allocate_info, nullptr, &memory));
    throw_if_failed(vkBindBufferMemory(device, buffer, memory, 0));
}

void vk_resource::destroy_buffer(VkBuffer buffer, VkDeviceMemory memory)
{
    VkDevice device = m_rhi->get_device();
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

void vk_resource::copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size)
{
    vk_graphics_queue* graphics_queue = m_rhi->get_graphics_queue();
    vk_command* command = graphics_queue->allocate_command();

    VkBufferCopy copy_region = {0, 0, size};
    vkCmdCopyBuffer(command->get_command_buffer(), source, target, 1, &copy_region);
    graphics_queue->execute_sync(command);
}

vk_image::vk_image(vk_rhi* rhi)
    : vk_resource(rhi),
      m_image(VK_NULL_HANDLE),
      m_memory(VK_NULL_HANDLE),
      m_image_view(VK_NULL_HANDLE),
      m_format(VK_FORMAT_UNDEFINED),
      m_extent{0, 0}
{
}

vk_image::~vk_image()
{
    destroy_image(m_image, m_memory);
    destroy_image_view(m_image_view);
}

VkImageView vk_image::get_image_view() const noexcept
{
    return m_image_view;
}

rhi_resource_format vk_image::get_format() const noexcept
{
    return vk_util::map_format(m_format);
}

rhi_resource_extent vk_image::get_extent() const noexcept
{
    return {m_extent.width, m_extent.height};
}

std::size_t vk_image::get_hash() const noexcept
{
    return m_hash;
}

void vk_image::set(
    VkImage image,
    VkDeviceMemory memory,
    VkImageView image_view,
    VkFormat format,
    VkExtent2D extent,
    std::size_t hash)
{
    m_image = image;
    m_memory = memory;
    m_image_view = image_view;
    m_format = format;
    m_extent = extent;
    m_hash = hash;
}

void vk_image::create_image(
    std::uint32_t width,
    std::uint32_t height,
    VkFormat format,
    VkSampleCountFlagBits samples,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkImage& image,
    VkDeviceMemory& memory)
{
    auto device = get_rhi()->get_device();

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = samples;
    image_info.flags = 0; // Optional

    throw_if_failed(vkCreateImage(device, &image_info, nullptr, &image));

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device, image, &requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex =
        find_memory_type(requirements.memoryTypeBits, properties, get_rhi()->get_physical_device());

    throw_if_failed(vkAllocateMemory(device, &allocate_info, nullptr, &memory));
    throw_if_failed(vkBindImageMemory(device, image, memory, 0));
}

void vk_image::destroy_image(VkImage image, VkDeviceMemory memory)
{
    VkDevice device = get_rhi()->get_device();
    vkDestroyImage(device, image, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

void vk_image::create_image_view(const VkImageViewCreateInfo& info, VkImageView& image_view)
{
    vkCreateImageView(get_rhi()->get_device(), &info, nullptr, &image_view);
}

void vk_image::destroy_image_view(VkImageView image_view)
{
    vkDestroyImageView(get_rhi()->get_device(), image_view, nullptr);
}

void vk_image::copy_buffer_to_image(
    VkBuffer buffer,
    VkImage image,
    std::uint32_t width,
    std::uint32_t height)
{
    vk_graphics_queue* graphics_queue = get_rhi()->get_graphics_queue();
    vk_command* command = graphics_queue->allocate_command();

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(
        command->get_command_buffer(),
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    graphics_queue->execute_sync(command);
}

void vk_image::transition_image_layout(
    VkImage image,
    VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    vk_graphics_queue* graphics_queue = get_rhi()->get_graphics_queue();
    vk_command* command = graphics_queue->allocate_command();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
        barrier.subresourceRange.aspectMask =
            VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    else
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (
        old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (
        old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else if (
        old_layout == VK_IMAGE_LAYOUT_UNDEFINED &&
        new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask =
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    else
    {
        throw vk_exception("Unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        command->get_command_buffer(),
        source_stage,
        destination_stage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    graphics_queue->execute_sync(command);
}

vk_swapchain_image::vk_swapchain_image(
    VkImage image,
    VkFormat format,
    const VkExtent2D& extent,
    vk_rhi* rhi)
    : vk_image(rhi)
{
    VkImageView image_view;

    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.image = image;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.format = format;
    image_view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    create_image_view(image_view_info, image_view);

    std::hash<void*> hasher;
    std::size_t hash = vk_util::hash_combine(hasher(image), hasher(image_view));
    hash = vk_util::hash_combine(hash, get_rhi()->get_frame_count());

    set(VK_NULL_HANDLE, VK_NULL_HANDLE, image_view, format, extent, hash);
}

vk_swapchain_image::~vk_swapchain_image()
{
}

vk_texture::vk_texture(const char* file, vk_rhi* rhi) : vk_image(rhi)
{
    vk_image_loader loader;
    if (!loader.load(file))
        throw vk_exception("Failed to load image.");

    const vk_image_data& data = loader.get_mipmap(0);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_memory;
    create_host_visible_buffer(
        data.pixels.data(),
        data.pixels.size(),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        staging_buffer,
        staging_memory);

    VkImage image;
    VkDeviceMemory memory;
    create_image(
        data.width,
        data.height,
        data.format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        image,
        memory);

    transition_image_layout(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer, image, data.width, data.height);
    transition_image_layout(
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    destroy_buffer(staging_buffer, staging_memory);

    VkImageView image_view;
    VkImageViewCreateInfo image_view_info = {};
    image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_info.image = image;
    image_view_info.format = data.format;
    image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_info.subresourceRange.baseMipLevel = 0;
    image_view_info.subresourceRange.levelCount = 1;
    image_view_info.subresourceRange.baseArrayLayer = 0;
    image_view_info.subresourceRange.layerCount = 1;
    create_image_view(image_view_info, image_view);

    std::hash<void*> hasher;
    std::size_t hash = vk_util::hash_combine(hasher(image), hasher(image_view));

    set(image, memory, image_view, data.format, VkExtent2D{data.width, data.height}, hash);
}

vk_texture::~vk_texture()
{
}

vk_buffer::vk_buffer(vk_rhi* rhi) : vk_resource(rhi)
{
}

vk_vertex_buffer::vk_vertex_buffer(const rhi_vertex_buffer_desc& desc, vk_rhi* rhi)
    : vk_buffer(rhi),
      m_mapping_pointer(nullptr)
{
    m_buffer_size = desc.size;

    VkDevice device = get_rhi()->get_device();
    if (desc.dynamic)
    {
        create_host_visible_buffer(
            desc.data,
            m_buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            m_buffer,
            m_memory);
        vkMapMemory(device, m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
    }
    else
    {
        create_device_local_buffer(
            desc.data,
            m_buffer_size,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            m_buffer,
            m_memory);
    }
}

vk_vertex_buffer::~vk_vertex_buffer()
{
    if (m_mapping_pointer)
        vkUnmapMemory(get_rhi()->get_device(), m_memory);

    destroy_buffer(m_buffer, m_memory);
}

vk_index_buffer::vk_index_buffer(const rhi_index_buffer_desc& desc, vk_rhi* rhi)
    : vk_buffer(rhi),
      m_mapping_pointer(nullptr)
{
    m_buffer_size = desc.size;

    if (desc.index_size == 2)
        m_index_type = VK_INDEX_TYPE_UINT16;
    else if (desc.index_size == 4)
        m_index_type = VK_INDEX_TYPE_UINT32;
    else
        throw vk_exception("Invalid index size.");

    VkDevice device = get_rhi()->get_device();
    if (desc.dynamic)
    {
        create_host_visible_buffer(
            desc.data,
            m_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            m_buffer,
            m_memory);

        vkMapMemory(device, m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
    }
    else
    {
        create_device_local_buffer(
            desc.data,
            m_buffer_size,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            m_buffer,
            m_memory);
    }
}

vk_index_buffer::~vk_index_buffer()
{
    if (m_mapping_pointer)
        vkUnmapMemory(get_rhi()->get_device(), m_memory);

    destroy_buffer(m_buffer, m_memory);
}

vk_uniform_buffer::vk_uniform_buffer(void* data, std::size_t size, vk_rhi* rhi)
    : vk_buffer(rhi),
      m_mapping_pointer(nullptr)
{
    m_buffer_size = size;

    create_host_visible_buffer(
        data,
        m_buffer_size,
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        m_buffer,
        m_memory);
    vkMapMemory(get_rhi()->get_device(), m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
}

vk_uniform_buffer::~vk_uniform_buffer()
{
    if (m_mapping_pointer)
        vkUnmapMemory(get_rhi()->get_device(), m_memory);

    destroy_buffer(m_buffer, m_memory);
}
} // namespace violet::vk
#include "vk_resource.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_image_loader.hpp"
#include "vk_renderer.hpp"

namespace ash::graphics::vk
{
std::pair<VkBuffer, VkDeviceMemory> vk_resource::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    std::pair<VkBuffer, VkDeviceMemory> result;

    auto device = vk_context::device();

    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throw_if_failed(vkCreateBuffer(device, &buffer_info, nullptr, &result.first));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, result.first, &requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, properties);

    throw_if_failed(vkAllocateMemory(device, &allocate_info, nullptr, &result.second));
    throw_if_failed(vkBindBufferMemory(device, result.first, result.second, 0));

    return result;
}

void vk_resource::copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size)
{
    auto& queue = vk_context::graphics_queue();

    VkCommandBuffer command_buffer = queue.begin_dynamic_command();

    VkBufferCopy copy_region = {0, 0, size};
    vkCmdCopyBuffer(command_buffer, source, target, 1, &copy_region);
    queue.end_dynamic_command(command_buffer);
}

std::uint32_t vk_resource::find_memory_type(
    std::uint32_t type_filter,
    VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memory_properties;
    vkGetPhysicalDeviceMemoryProperties(vk_context::physical_device(), &memory_properties);

    for (std::uint32_t i = 0; i < memory_properties.memoryTypeCount; ++i)
    {
        if ((type_filter & (1 << i)) &&
            (memory_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw vk_exception("failed to find suitable memory type!");
}

std::pair<VkImage, VkDeviceMemory> vk_image::create_image(
    std::uint32_t width,
    std::uint32_t height,
    VkFormat format,
    std::size_t samples,
    VkImageTiling tiling,
    VkImageUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    std::pair<VkImage, VkDeviceMemory> result;

    auto device = vk_context::device();

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = to_vk_samples(samples);
    image_info.flags = 0; // Optional

    throw_if_failed(vkCreateImage(device, &image_info, nullptr, &result.first));

    VkMemoryRequirements requirements;
    vkGetImageMemoryRequirements(device, result.first, &requirements);

    VkMemoryAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocate_info.allocationSize = requirements.size;
    allocate_info.memoryTypeIndex = find_memory_type(requirements.memoryTypeBits, properties);

    throw_if_failed(vkAllocateMemory(device, &allocate_info, nullptr, &result.second));
    throw_if_failed(vkBindImageMemory(device, result.first, result.second, 0));

    return result;
}

VkImageView vk_image::create_image_view(
    VkImage image,
    VkFormat format,
    VkImageAspectFlags aspect_mask)
{
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.image = image;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_mask;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView result;
    throw_if_failed(vkCreateImageView(vk_context::device(), &view_info, nullptr, &result));
    return result;
}

void vk_image::copy_buffer_to_image(
    VkBuffer buffer,
    VkImage image,
    std::uint32_t width,
    std::uint32_t height)
{
    auto& queue = vk_context::graphics_queue();
    VkCommandBuffer command_buffer = queue.begin_dynamic_command();

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
        command_buffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);

    queue.end_dynamic_command(command_buffer);
}

void vk_image::transition_image_layout(
    VkImage image,
    VkFormat format,
    VkImageLayout old_layout,
    VkImageLayout new_layout)
{
    auto& queue = vk_context::graphics_queue();
    VkCommandBuffer command_buffer = queue.begin_dynamic_command();

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
        command_buffer,
        source_stage,
        destination_stage,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    queue.end_dynamic_command(command_buffer);
}

vk_back_buffer::vk_back_buffer(VkImage image, VkFormat format, const VkExtent2D& extent)
    : m_image(image)
{
    m_image_view = create_image_view(image, format, VK_IMAGE_ASPECT_COLOR_BIT);
    m_format = to_ash_format(format);
    m_extent = extent;

    transition_image_layout(
        m_image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
}

vk_back_buffer::vk_back_buffer(vk_back_buffer&& other)
    : m_image_view(other.m_image_view),
      m_image(other.m_image),
      m_format(other.m_format),
      m_extent(other.m_extent)
{
    other.m_image_view = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_format = resource_format::UNDEFINED;
    other.m_extent = {};
}

vk_back_buffer::~vk_back_buffer()
{
    destroy();
}

vk_back_buffer& vk_back_buffer::operator=(vk_back_buffer&& other)
{
    if (this != &other)
    {
        destroy();

        m_image_view = other.m_image_view;
        m_image = other.m_image;
        m_format = other.m_format;
        m_extent = other.m_extent;

        other.m_image_view = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_format = resource_format::UNDEFINED;
        other.m_extent = {};
    }

    return *this;
}

void vk_back_buffer::destroy()
{
    if (m_image_view != VK_NULL_HANDLE)
    {
        vk_context::frame_buffer().notify_destroy(this);
        auto device = vk_context::device();
        vkDestroyImageView(device, m_image_view, nullptr);
    }

    m_image_view = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_format = resource_format::UNDEFINED;
    m_extent = {};
}

vk_render_target::vk_render_target(
    std::uint32_t width,
    std::uint32_t height,
    VkFormat format,
    std::size_t samples)
{
    auto [image, image_memory] = create_image(
        width,
        height,
        format,
        samples,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_image = image;
    m_image_memory = image_memory;

    m_image_view = create_image_view(m_image, format, VK_IMAGE_ASPECT_COLOR_BIT);

    transition_image_layout(
        m_image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    m_extent.width = width;
    m_extent.height = height;
    m_format = to_ash_format(format);
}

vk_render_target::vk_render_target(const render_target_desc& desc)
    : vk_render_target(desc.width, desc.height, to_vk_format(desc.format), desc.samples)
{
}

vk_render_target::vk_render_target(vk_render_target&& other)
    : m_image_view(other.m_image_view),
      m_image(other.m_image),
      m_image_memory(other.m_image_memory),
      m_format(other.m_format),
      m_extent(other.m_extent)
{
    other.m_image_view = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_image_memory = VK_NULL_HANDLE;
    other.m_format = resource_format::UNDEFINED;
    other.m_extent = {};
}

vk_render_target::~vk_render_target()
{
    destroy();
}

vk_render_target& vk_render_target::operator=(vk_render_target&& other)
{
    if (this != &other)
    {
        destroy();

        m_image_view = other.m_image_view;
        m_image = other.m_image;
        m_image_memory = other.m_image_memory;
        m_format = other.m_format;
        m_extent = other.m_extent;

        other.m_image_view = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_image_memory = VK_NULL_HANDLE;
        other.m_format = resource_format::UNDEFINED;
        other.m_extent = {};
    }

    return *this;
}

void vk_render_target::destroy()
{
    if (m_image_view != VK_NULL_HANDLE)
    {
        vk_context::frame_buffer().notify_destroy(this);

        auto device = vk_context::device();
        vkDestroyImageView(device, m_image_view, nullptr);
        vkDestroyImage(device, m_image, nullptr);
        vkFreeMemory(device, m_image_memory, nullptr);
    }

    m_image_view = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_image_memory = VK_NULL_HANDLE;
    m_format = resource_format::UNDEFINED;
    m_extent = {};
}

vk_depth_stencil_buffer::vk_depth_stencil_buffer(
    std::uint32_t width,
    std::uint32_t height,
    VkFormat format,
    std::size_t samples)
{
    auto [image, image_memory] = create_image(
        width,
        height,
        format,
        samples,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_image = image;
    m_image_memory = image_memory;

    m_image_view = create_image_view(m_image, format, VK_IMAGE_ASPECT_DEPTH_BIT);

    transition_image_layout(
        m_image,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    m_extent.width = width;
    m_extent.height = height;
    m_format = to_ash_format(format);
}

vk_depth_stencil_buffer::vk_depth_stencil_buffer(const depth_stencil_buffer_desc& desc)
    : vk_depth_stencil_buffer(desc.width, desc.height, to_vk_format(desc.format), desc.samples)
{
}

vk_depth_stencil_buffer::vk_depth_stencil_buffer(vk_depth_stencil_buffer&& other)
    : m_image_view(other.m_image_view),
      m_image(other.m_image),
      m_image_memory(other.m_image_memory),
      m_format(other.m_format),
      m_extent(other.m_extent)
{
    other.m_image_view = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_image_memory = VK_NULL_HANDLE;
    other.m_format = resource_format::UNDEFINED;
    other.m_extent = {};
}

vk_depth_stencil_buffer::~vk_depth_stencil_buffer()
{
    destroy();
}

vk_depth_stencil_buffer& vk_depth_stencil_buffer::operator=(vk_depth_stencil_buffer&& other)
{
    if (this != &other)
    {
        destroy();

        m_image_view = other.m_image_view;
        m_image = other.m_image;
        m_image_memory = other.m_image_memory;
        m_format = other.m_format;
        m_extent = other.m_extent;

        other.m_image_view = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_image_memory = VK_NULL_HANDLE;
        other.m_format = resource_format::UNDEFINED;
        other.m_extent = {};
    }

    return *this;
}

void vk_depth_stencil_buffer::destroy()
{
    if (m_image_view != VK_NULL_HANDLE)
    {
        auto device = vk_context::device();
        vkDestroyImageView(device, m_image_view, nullptr);
        vkDestroyImage(device, m_image, nullptr);
        vkFreeMemory(device, m_image_memory, nullptr);
    }

    m_image_view = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_image_memory = VK_NULL_HANDLE;
    m_format = resource_format::UNDEFINED;
    m_extent = {};
}

vk_texture::vk_texture(const std::uint8_t* data, std::uint32_t width, std::uint32_t height)
{
}

vk_texture::vk_texture(std::string_view file)
{
    vk_image_loader loader;
    if (!loader.load(file))
        throw vk_exception("Load texture failed.");

    auto& data = loader.mipmap(0);

    auto [staging_buffer, staging_buffer_memory] = create_buffer(
        static_cast<VkDeviceSize>(data.pixels.size()),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    auto device = vk_context::device();

    void* mapping;
    vkMapMemory(device, staging_buffer_memory, 0, data.pixels.size(), 0, &mapping);
    std::memcpy(mapping, data.pixels.data(), data.pixels.size());
    vkUnmapMemory(device, staging_buffer_memory);

    auto [image, image_memory] = create_image(
        data.width,
        data.height,
        data.format,
        1,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    transition_image_layout(
        image,
        data.format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(staging_buffer, image, data.width, data.height);
    transition_image_layout(
        image,
        data.format,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    // Create view.
    m_image_view = create_image_view(image, data.format, VK_IMAGE_ASPECT_COLOR_BIT);

    m_image = image;
    m_image_memory = image_memory;

    m_extent.width = data.width;
    m_extent.height = data.height;
}

vk_texture::vk_texture(vk_texture&& other)
    : m_image_view(other.m_image_view),
      m_image(other.m_image),
      m_image_memory(other.m_image_memory),
      m_format(other.m_format),
      m_extent(other.m_extent)
{
    other.m_image_view = VK_NULL_HANDLE;
    other.m_image = VK_NULL_HANDLE;
    other.m_image_memory = VK_NULL_HANDLE;
    other.m_format = resource_format::UNDEFINED;
    other.m_extent = {};
}

vk_texture::~vk_texture()
{
    destroy();
}

vk_texture& vk_texture::operator=(vk_texture&& other)
{
    if (this != &other)
    {
        destroy();

        m_image_view = other.m_image_view;
        m_image = other.m_image;
        m_image_memory = other.m_image_memory;
        m_format = other.m_format;
        m_extent = other.m_extent;

        other.m_image_view = VK_NULL_HANDLE;
        other.m_image = VK_NULL_HANDLE;
        other.m_image_memory = VK_NULL_HANDLE;
        other.m_format = resource_format::UNDEFINED;
        other.m_extent = {};
    }

    return *this;
}

void vk_texture::destroy()
{
    if (m_image_view != VK_NULL_HANDLE)
    {
        auto device = vk_context::device();
        vkDestroyImageView(device, m_image_view, nullptr);
        vkDestroyImage(device, m_image, nullptr);
        vkFreeMemory(device, m_image_memory, nullptr);
    }

    m_image_view = VK_NULL_HANDLE;
    m_image = VK_NULL_HANDLE;
    m_image_memory = VK_NULL_HANDLE;
    m_format = resource_format::UNDEFINED;
    m_extent = {};
}

vk_device_local_buffer::vk_device_local_buffer(
    const void* data,
    std::size_t size,
    VkBufferUsageFlags flags)
{
    auto device = vk_context::device();

    std::uint32_t buffer_size = static_cast<std::uint32_t>(size);

    auto [staging_buffer, staging_buffer_memory] = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mapping;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &mapping);
    std::memcpy(mapping, data, buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    auto [vertex_buffer, vertex_buffer_memory] = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | flags,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(staging_buffer, vertex_buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    m_buffer = vertex_buffer;
    m_buffer_memory = vertex_buffer_memory;
    m_buffer_size = size;
}

vk_device_local_buffer::vk_device_local_buffer(vk_device_local_buffer&& other)
    : m_buffer(other.m_buffer),
      m_buffer_memory(other.m_buffer_memory),
      m_buffer_size(other.m_buffer_size)
{
    other.m_buffer = VK_NULL_HANDLE;
    other.m_buffer_memory = VK_NULL_HANDLE;
    other.m_buffer_size = 0;
}

vk_device_local_buffer::~vk_device_local_buffer()
{
    destroy();
}

vk_device_local_buffer& vk_device_local_buffer::operator=(vk_device_local_buffer&& other)
{
    if (this != &other)
    {
        destroy();

        m_buffer = other.m_buffer;
        m_buffer_memory = other.m_buffer_memory;
        m_buffer_size = other.m_buffer_size;

        other.m_buffer = VK_NULL_HANDLE;
        other.m_buffer_memory = VK_NULL_HANDLE;
        other.m_buffer_size = 0;
    }

    return *this;
}

void vk_device_local_buffer::destroy()
{
    if (m_buffer != VK_NULL_HANDLE)
    {
        auto device = vk_context::device();
        vkDestroyBuffer(device, m_buffer, nullptr);
        vkFreeMemory(device, m_buffer_memory, nullptr);
    }

    m_buffer = VK_NULL_HANDLE;
    m_buffer_memory = VK_NULL_HANDLE;
    m_buffer_size = 0;
}

vk_host_visible_buffer::vk_host_visible_buffer(
    const void* data,
    std::size_t size,
    VkBufferUsageFlags flags)
{
    auto device = vk_context::device();

    std::uint32_t buffer_size = static_cast<std::uint32_t>(size);

    auto [vertex_buffer, vertex_buffer_memory] = create_buffer(
        buffer_size,
        flags,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    m_buffer = vertex_buffer;
    m_buffer_memory = vertex_buffer_memory;
    m_buffer_size = size;

    if (data != nullptr)
        upload(data, size, 0);
}

vk_host_visible_buffer::vk_host_visible_buffer(vk_host_visible_buffer&& other)
    : m_buffer(other.m_buffer),
      m_buffer_memory(other.m_buffer_memory),
      m_buffer_size(other.m_buffer_size)
{
    other.m_buffer = VK_NULL_HANDLE;
    other.m_buffer_memory = VK_NULL_HANDLE;
    other.m_buffer_size = 0;
}

vk_host_visible_buffer::~vk_host_visible_buffer()
{
    destroy();
}

void vk_host_visible_buffer::upload(const void* data, std::size_t size, std::size_t offset)
{
    auto device = vk_context::device();

    void* mapping;
    vkMapMemory(device, m_buffer_memory, 0, static_cast<std::uint32_t>(size), 0, &mapping);
    std::memcpy(mapping, data, static_cast<std::uint32_t>(size));
    vkUnmapMemory(device, m_buffer_memory);
}

vk_host_visible_buffer& vk_host_visible_buffer::operator=(vk_host_visible_buffer&& other)
{
    if (this != &other)
    {
        destroy();

        m_buffer = other.m_buffer;
        m_buffer_memory = other.m_buffer_memory;
        m_buffer_size = other.m_buffer_size;

        other.m_buffer = VK_NULL_HANDLE;
        other.m_buffer_memory = VK_NULL_HANDLE;
        other.m_buffer_size = 0;
    }

    return *this;
}

void vk_host_visible_buffer::destroy()
{
    if (m_buffer != VK_NULL_HANDLE)
    {
        auto device = vk_context::device();
        vkDestroyBuffer(device, m_buffer, nullptr);
        vkFreeMemory(device, m_buffer_memory, nullptr);
    }

    m_buffer = VK_NULL_HANDLE;
    m_buffer_memory = VK_NULL_HANDLE;
    m_buffer_size = 0;
}

vk_uniform_buffer::vk_uniform_buffer(std::size_t size) : m_buffer_size(size)
{
    auto [buffer, buffer_memory] = create_buffer(
        static_cast<std::uint32_t>(size),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    m_buffer = buffer;
    m_buffer_memory = buffer_memory;
}

vk_uniform_buffer::vk_uniform_buffer(vk_uniform_buffer&& other)
    : m_buffer(other.m_buffer),
      m_buffer_memory(other.m_buffer_memory),
      m_buffer_size(other.m_buffer_size)
{
    other.m_buffer = VK_NULL_HANDLE;
    other.m_buffer_memory = VK_NULL_HANDLE;
    other.m_buffer_size = 0;
}

vk_uniform_buffer::~vk_uniform_buffer()
{
    destroy();
}

void vk_uniform_buffer::upload(const void* data, std::size_t size, std::size_t offset)
{
    auto device = vk_context::device();

    void* mapping;
    vkMapMemory(
        device,
        m_buffer_memory,
        static_cast<std::uint32_t>(offset),
        static_cast<std::uint32_t>(size),
        0,
        &mapping);
    std::memcpy(mapping, data, size);
    vkUnmapMemory(device, m_buffer_memory);
}

vk_uniform_buffer& vk_uniform_buffer::operator=(vk_uniform_buffer&& other)
{
    if (this != &other)
    {
        destroy();

        m_buffer = other.m_buffer;
        m_buffer_memory = other.m_buffer_memory;
        m_buffer_size = other.m_buffer_size;

        other.m_buffer = VK_NULL_HANDLE;
        other.m_buffer_memory = VK_NULL_HANDLE;
        other.m_buffer_size = 0;
    }

    return *this;
}

void vk_uniform_buffer::destroy()
{
    auto device = vk_context::device();
    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_buffer_memory, nullptr);
}
} // namespace ash::graphics::vk
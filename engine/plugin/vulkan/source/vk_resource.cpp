#include "vk_resource.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

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

std::pair<VkImage, VkDeviceMemory> vk_resource::create_image(
    std::uint32_t width,
    std::uint32_t height,
    VkFormat format,
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
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
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

VkImageView vk_resource::create_image_view(VkImage image, VkFormat format)
{
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.image = image;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView result;
    throw_if_failed(vkCreateImageView(vk_context::device(), &view_info, nullptr, &result));
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

void vk_resource::copy_buffer_to_image(
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

void vk_resource::transition_image_layout(
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
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

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

vk_back_buffer::vk_back_buffer(VkImage image, VkFormat format) : m_image(image)
{
    m_view = create_image_view(image, format);
}

vk_back_buffer::vk_back_buffer(vk_back_buffer&& other) : m_view(other.m_view)
{
    other.m_view = VK_NULL_HANDLE;
}

vk_back_buffer::~vk_back_buffer()
{
    if (m_view != VK_NULL_HANDLE)
    {
        auto device = vk_context::device();
        vkDestroyImageView(device, m_view, nullptr);
    }
}

vk_back_buffer& vk_back_buffer::operator=(vk_back_buffer&& other)
{
    m_view = other.m_view;
    other.m_view = VK_NULL_HANDLE;

    return *this;
}

vk_vertex_buffer::vk_vertex_buffer(const vertex_buffer_desc& desc)
{
    auto device = vk_context::device();

    std::uint32_t buffer_size = static_cast<std::uint32_t>(desc.vertex_size * desc.vertex_count);

    auto [staging_buffer, staging_buffer_memory] = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mapping;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &mapping);
    std::memcpy(mapping, desc.vertices, buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    auto [vertex_buffer, vertex_buffer_memory] = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(staging_buffer, vertex_buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    m_buffer = vertex_buffer;
    m_buffer_memory = vertex_buffer_memory;
}

vk_vertex_buffer::~vk_vertex_buffer()
{
    auto device = vk_context::device();
    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_buffer_memory, nullptr);
}

vk_index_buffer::vk_index_buffer(const index_buffer_desc& desc)
{
    auto device = vk_context::device();

    std::uint32_t buffer_size = static_cast<std::uint32_t>(desc.index_size * desc.index_count);

    auto [staging_buffer, staging_buffer_memory] = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* mapping;
    vkMapMemory(device, staging_buffer_memory, 0, buffer_size, 0, &mapping);
    std::memcpy(mapping, desc.indices, buffer_size);
    vkUnmapMemory(device, staging_buffer_memory);

    auto [index_buffer, index_buffer_memory] = create_buffer(
        buffer_size,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(staging_buffer, index_buffer, buffer_size);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    m_buffer = index_buffer;
    m_buffer_memory = index_buffer_memory;

    switch (desc.index_size)
    {
    case 2:
        m_index_type = VK_INDEX_TYPE_UINT16;
        break;
    case 4:
        m_index_type = VK_INDEX_TYPE_UINT32;
        break;
    default:
        throw vk_exception("Invalid index size.");
    }
}

vk_index_buffer::~vk_index_buffer()
{
    auto device = vk_context::device();
    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_buffer_memory, nullptr);
}

vk_uniform_buffer::vk_uniform_buffer(std::size_t size)
{
    auto [buffer, buffer_memory] = create_buffer(
        static_cast<std::uint32_t>(size),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    m_buffer = buffer;
    m_buffer_memory = buffer_memory;
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

vk_uniform_buffer::~vk_uniform_buffer()
{
    auto device = vk_context::device();
    vkDestroyBuffer(device, m_buffer, nullptr);
    vkFreeMemory(device, m_buffer_memory, nullptr);
}

vk_texture::vk_texture(std::string_view file)
{
    int width, height, channels;
    stbi_uc* pixels = stbi_load(file.data(), &width, &height, &channels, STBI_rgb_alpha);
    std::size_t image_size = width * height * 4;

    auto [staging_buffer, staging_buffer_memory] = create_buffer(
        static_cast<VkDeviceSize>(image_size),
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    auto device = vk_context::device();

    void* mapping;
    vkMapMemory(device, staging_buffer_memory, 0, image_size, 0, &mapping);
    std::memcpy(mapping, pixels, image_size);
    vkUnmapMemory(device, staging_buffer_memory);

    stbi_image_free(pixels);

    auto [image, image_memory] = create_image(
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height),
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    transition_image_layout(
        image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copy_buffer_to_image(
        staging_buffer,
        image,
        static_cast<std::uint32_t>(width),
        static_cast<std::uint32_t>(height));
    transition_image_layout(
        image,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(device, staging_buffer, nullptr);
    vkFreeMemory(device, staging_buffer_memory, nullptr);

    // Create view.
    m_image_view = create_image_view(image, VK_FORMAT_R8G8B8A8_SRGB);

    m_image = image;
    m_image_memory = image_memory;
}

vk_texture::~vk_texture()
{
    auto device = vk_context::device();
    vkDestroyImage(device, m_image, nullptr);
    vkFreeMemory(device, m_image_memory, nullptr);
}
} // namespace ash::graphics::vk
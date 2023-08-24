#include "vk_resource.hpp"
#include "vk_command.hpp"
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

    throw vk_exception("failed to find suitable memory type!");
}
} // namespace

vk_resource::vk_resource(vk_rhi* rhi) : m_rhi(rhi)
{
}

vk_image::vk_image(vk_rhi* rhi) : vk_resource(rhi)
{
}

void vk_image::create_image_view(VkImageViewCreateInfo* info, VkImageView& image_view)
{
    vkCreateImageView(get_rhi()->get_device(), info, nullptr, &image_view);
}

void vk_image::destroy_image_view(VkImageView image_view)
{
    vkDestroyImageView(get_rhi()->get_device(), image_view, nullptr);
}

vk_swapchain_image::vk_swapchain_image(
    VkImage image,
    VkFormat format,
    const VkExtent2D& extent,
    vk_rhi* rhi)
    : vk_image(rhi),
      m_image(image),
      m_format(format),
      m_extent(extent)
{
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

    create_image_view(&image_view_info, m_image_view);
}

vk_swapchain_image::~vk_swapchain_image()
{
    destroy_image_view(m_image_view);
}

VkImageView vk_swapchain_image::get_image_view() const noexcept
{
    return m_image_view;
}

rhi_resource_format vk_swapchain_image::get_format() const noexcept
{
    return vk_util::map_format(m_format);
}

rhi_resource_extent vk_swapchain_image::get_extent() const noexcept
{
    return {m_extent.width, m_extent.height};
}

std::size_t vk_swapchain_image::get_hash() const noexcept
{
    std::hash<void*> hasher;
    return hasher(m_image);
}

vk_buffer::vk_buffer(vk_rhi* rhi) : vk_resource(rhi)
{
}

void vk_buffer::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties,
    VkBuffer& buffer,
    VkDeviceMemory& memory)
{
    VkDevice device = get_rhi()->get_device();

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
        find_memory_type(requirements.memoryTypeBits, properties, get_rhi()->get_physical_device());

    throw_if_failed(vkAllocateMemory(device, &allocate_info, nullptr, &memory));
    throw_if_failed(vkBindBufferMemory(device, buffer, memory, 0));
}

void vk_buffer::destroy_buffer(VkBuffer buffer, VkDeviceMemory memory)
{
    VkDevice device = get_rhi()->get_device();
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, memory, nullptr);
}

void vk_buffer::copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size)
{
    vk_command_queue* graphics_queue = get_rhi()->get_graphics_queue();
    vk_command* command = graphics_queue->allocate_command();

    VkBufferCopy copy_region = {0, 0, size};
    vkCmdCopyBuffer(command->get_command_buffer(), source, target, 1, &copy_region);
    graphics_queue->execute_sync(command);
}

vk_vertex_buffer::vk_vertex_buffer(const rhi_vertex_buffer_desc& desc, vk_rhi* rhi)
    : vk_buffer(rhi),
      m_mapping_pointer(nullptr)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = desc.size;
    buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkMemoryPropertyFlags flags;
    if (desc.dynamic)
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    create_buffer(desc.size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, flags, m_buffer, m_memory);
    m_buffer_size = desc.size;

    if (desc.dynamic)
    {
        vkMapMemory(get_rhi()->get_device(), m_memory, 0, m_buffer_size, 0, &m_mapping_pointer);
        if (desc.data != nullptr)
            std::memcpy(m_mapping_pointer, desc.data, m_buffer_size);
    }
}

vk_vertex_buffer::~vk_vertex_buffer()
{
    destroy_buffer(m_buffer, m_memory);
}

std::size_t vk_vertex_buffer::get_hash() const noexcept
{
    std::hash<void*> hasher;
    return hasher(m_buffer);
}
} // namespace violet::vk
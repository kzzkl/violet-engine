#include "vk_resource.hpp"
#include "vk_command.hpp"
#include "vk_context.hpp"

namespace ash::graphics::vk
{
std::pair<VkBuffer, VkDeviceMemory> vk_buffer::create_buffer(
    VkDeviceSize size,
    VkBufferUsageFlags usage,
    VkMemoryPropertyFlags properties)
{
    std::pair<VkBuffer, VkDeviceMemory> result;

    auto device = vk_context::device();

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    throw_if_failed(vkCreateBuffer(device, &bufferInfo, nullptr, &result.first));

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, result.first, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, properties);

    throw_if_failed(vkAllocateMemory(device, &allocInfo, nullptr, &result.second));

    vkBindBufferMemory(device, result.first, result.second, 0);

    return result;
}

void vk_buffer::copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size)
{
    auto& queue = vk_context::graphics_queue();

    VkCommandBuffer command_buffer = queue.begin_dynamic_command();

    VkBufferCopy copy_region = {0, 0, size};
    vkCmdCopyBuffer(command_buffer, source, target, 1, &copy_region);
    queue.end_dynamic_command(command_buffer);
}

std::uint32_t vk_buffer::find_memory_type(
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
}
} // namespace ash::graphics::vk
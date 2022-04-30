#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_image : public resource
{
public:
    vk_image(VkImage image, VkImageView view) : m_image(image), m_view(view) {}

    VkImageView view() const noexcept { return m_view; }

private:
    VkImage m_image;
    VkImageView m_view;
};

class vk_buffer : public resource
{
public:
protected:
    std::pair<VkBuffer, VkDeviceMemory> create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    void copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size);

    std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);
};

class vk_vertex_buffer : public vk_buffer
{
public:
    vk_vertex_buffer(const vertex_buffer_desc& desc);
    virtual ~vk_vertex_buffer();

    VkBuffer buffer() const noexcept { return m_buffer; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;
};

class vk_index_buffer : public vk_buffer
{
public:
    vk_index_buffer(const index_buffer_desc& desc);
    virtual ~vk_index_buffer();

    VkBuffer buffer() const noexcept { return m_buffer; }
    VkIndexType index_type() const noexcept { return m_index_type; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;

    VkIndexType m_index_type;
};
} // namespace ash::graphics::vk
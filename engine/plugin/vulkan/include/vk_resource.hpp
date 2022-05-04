#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_resource : public resource_interface
{
public:
protected:
    std::pair<VkBuffer, VkDeviceMemory> create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    std::pair<VkImage, VkDeviceMemory> create_image(
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);

    VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_mask);

    void copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size);
    void copy_buffer_to_image(
        VkBuffer buffer,
        VkImage image,
        std::uint32_t width,
        std::uint32_t height);

    void transition_image_layout(
        VkImage image,
        VkFormat format,
        VkImageLayout old_layout,
        VkImageLayout new_layout);

    std::uint32_t find_memory_type(std::uint32_t type_filter, VkMemoryPropertyFlags properties);
};

class vk_image : public vk_resource
{
public:
    virtual VkImageView view() const noexcept = 0;
};

class vk_back_buffer : public vk_image
{
public:
    vk_back_buffer(VkImage image, VkFormat format);
    vk_back_buffer(vk_back_buffer&& other);
    virtual ~vk_back_buffer();

    virtual VkImageView view() const noexcept override { return m_image_view; }

    vk_back_buffer& operator=(vk_back_buffer&& other);

private:
    VkImageView m_image_view;
    VkImage m_image;
};

class vk_depth_stencil_buffer : public vk_image
{
public:
    vk_depth_stencil_buffer(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling);

    virtual VkImageView view() const noexcept override { return m_image_view; }

private:
    VkImageView m_image_view;

    VkImage m_image;
    VkDeviceMemory m_image_memory;
};

class vk_vertex_buffer : public vk_resource
{
public:
    vk_vertex_buffer(const vertex_buffer_desc& desc);
    virtual ~vk_vertex_buffer();

    VkBuffer buffer() const noexcept { return m_buffer; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;
};

class vk_index_buffer : public vk_resource
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

class vk_uniform_buffer : public vk_resource
{
public:
    vk_uniform_buffer(std::size_t size);
    virtual ~vk_uniform_buffer();

    void upload(const void* data, std::size_t size, std::size_t offset);
    VkBuffer buffer() const noexcept { return m_buffer; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;
};

class vk_texture : public vk_image
{
public:
    vk_texture(std::string_view file);
    virtual ~vk_texture();

    virtual VkImageView view() const noexcept override { return m_image_view; }

public:
    VkImageView m_image_view;

    VkImage m_image;
    VkDeviceMemory m_image_memory;
};
} // namespace ash::graphics::vk
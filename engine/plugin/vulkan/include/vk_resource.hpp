#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_resource : public resource_interface
{
public:
    virtual resource_format format() const noexcept override { return resource_format::UNDEFINED; }
    virtual resource_extent extent() const noexcept override { return {0, 0}; }
    virtual std::size_t size() const noexcept override { return 0; }

protected:
    static std::pair<VkBuffer, VkDeviceMemory> create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties);

    static void copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size);

    static std::uint32_t find_memory_type(
        std::uint32_t type_filter,
        VkMemoryPropertyFlags properties);
};

class vk_image : public vk_resource
{
public:
    virtual VkImageView view() const noexcept = 0;
    virtual VkClearValue clear_value() const noexcept
    {
        VkClearValue value = {};
        value.color = {0.0f, 0.0f, 0.0f, 1.0f};
        return value;
    }

protected:
    static std::pair<VkImage, VkDeviceMemory> create_image(
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        std::size_t samples,
        VkImageTiling tiling,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties);

    static VkImageView create_image_view(
        VkImage image,
        VkFormat format,
        VkImageAspectFlags aspect_mask);

    static void copy_buffer_to_image(
        VkBuffer buffer,
        VkImage image,
        std::uint32_t width,
        std::uint32_t height);

    static void transition_image_layout(
        VkImage image,
        VkFormat format,
        VkImageLayout old_layout,
        VkImageLayout new_layout);

private:
    VkImageView m_image_view;
    VkImage m_image;
    VkDeviceMemory m_image_memory;

    resource_format m_format;
    VkExtent2D m_extent;
};

class vk_back_buffer : public vk_image
{
public:
    vk_back_buffer(VkImage image, VkFormat format, const VkExtent2D& extent);
    vk_back_buffer(vk_back_buffer&& other);
    virtual ~vk_back_buffer();

    virtual VkImageView view() const noexcept override { return m_image_view; }

    virtual resource_format format() const noexcept override { return m_format; }
    virtual resource_extent extent() const noexcept override
    {
        return resource_extent{m_extent.width, m_extent.height};
    }

    vk_back_buffer& operator=(vk_back_buffer&& other);

private:
    void destroy();

    VkImageView m_image_view;
    VkImage m_image;

    resource_format m_format;
    VkExtent2D m_extent;
};

class vk_render_target : public vk_image
{
public:
    vk_render_target(
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        std::size_t samples);
    vk_render_target(const render_target_desc& desc);
    vk_render_target(vk_render_target&& other);

    virtual ~vk_render_target();

    virtual VkImageView view() const noexcept override { return m_image_view; }

    virtual resource_format format() const noexcept override { return m_format; }
    virtual resource_extent extent() const noexcept override
    {
        return resource_extent{m_extent.width, m_extent.height};
    }

    vk_render_target& operator=(vk_render_target&& other);

private:
    void destroy();

    VkImageView m_image_view;
    VkImage m_image;
    VkDeviceMemory m_image_memory;

    resource_format m_format;
    VkExtent2D m_extent;
};

class vk_depth_stencil_buffer : public vk_image
{
public:
    vk_depth_stencil_buffer(
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        std::size_t samples);
    vk_depth_stencil_buffer(const depth_stencil_buffer_desc& desc);
    vk_depth_stencil_buffer(vk_depth_stencil_buffer&& other);
    virtual ~vk_depth_stencil_buffer();

    virtual VkImageView view() const noexcept override { return m_image_view; }

    virtual resource_format format() const noexcept override { return m_format; }
    virtual resource_extent extent() const noexcept override
    {
        return resource_extent{m_extent.width, m_extent.height};
    }

    virtual VkClearValue clear_value() const noexcept
    {
        VkClearValue value = {};
        value.depthStencil = {1.0f, 0};
        return value;
    }

    vk_depth_stencil_buffer& operator=(vk_depth_stencil_buffer&& other);

private:
    void destroy();

    VkImageView m_image_view;
    VkImage m_image;
    VkDeviceMemory m_image_memory;

    resource_format m_format;
    VkExtent2D m_extent;
};

class vk_texture : public vk_image
{
public:
    vk_texture(std::string_view file);
    vk_texture(vk_texture&& other);
    virtual ~vk_texture();

    virtual VkImageView view() const noexcept override { return m_image_view; }

    virtual resource_format format() const noexcept override { return m_format; }
    virtual resource_extent extent() const noexcept override
    {
        return resource_extent{m_extent.width, m_extent.height};
    }

    vk_texture& operator=(vk_texture&& other);

public:
    void destroy();

    VkImageView m_image_view;

    VkImage m_image;
    VkDeviceMemory m_image_memory;

    resource_format m_format;
    VkExtent2D m_extent;
};

class vk_vertex_buffer : public vk_resource
{
public:
    vk_vertex_buffer(const vertex_buffer_desc& desc);
    vk_vertex_buffer(vk_vertex_buffer&& other);
    virtual ~vk_vertex_buffer();

    VkBuffer buffer() const noexcept { return m_buffer; }
    virtual std::size_t size() const noexcept override { return m_buffer_size; }

    vk_vertex_buffer& operator=(vk_vertex_buffer&& other);

private:
    void destroy();

    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;
    std::size_t m_buffer_size;
};

class vk_index_buffer : public vk_resource
{
public:
    vk_index_buffer(const index_buffer_desc& desc);
    vk_index_buffer(vk_index_buffer&& other);
    virtual ~vk_index_buffer();

    VkBuffer buffer() const noexcept { return m_buffer; }
    VkIndexType index_type() const noexcept { return m_index_type; }

    vk_index_buffer& operator=(vk_index_buffer&& other);

private:
    void destroy();

    VkIndexType m_index_type;

    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;
    std::size_t m_buffer_size;
};

class vk_uniform_buffer : public vk_resource
{
public:
    vk_uniform_buffer(std::size_t size);
    vk_uniform_buffer(vk_uniform_buffer&& other);
    virtual ~vk_uniform_buffer();

    void upload(const void* data, std::size_t size, std::size_t offset);
    VkBuffer buffer() const noexcept { return m_buffer; }

    vk_uniform_buffer& operator=(vk_uniform_buffer&& other);

private:
    void destroy();

    VkBuffer m_buffer;
    VkDeviceMemory m_buffer_memory;
    std::size_t m_buffer_size;
};
} // namespace ash::graphics::vk
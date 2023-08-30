#pragma once

#include "vk_common.hpp"
#include <vector>

namespace violet::vk
{
class vk_rhi;
class vk_resource : public rhi_resource
{
public:
    vk_resource(vk_rhi* rhi);
    vk_resource(const vk_resource&) = delete;
    virtual ~vk_resource() = default;

    virtual rhi_resource_format get_format() const noexcept override
    {
        return RHI_RESOURCE_FORMAT_UNDEFINED;
    }
    virtual rhi_resource_extent get_extent() const noexcept override { return {0, 0}; }

    virtual void* get_buffer() { return nullptr; }
    virtual std::size_t get_buffer_size() const noexcept override { return 0; }

    vk_resource& operator=(const vk_resource&) = delete;

protected:
    void create_device_local_buffer(
        const void* data,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    void create_host_visible_buffer(
        const void* data,
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkBuffer& buffer,
        VkDeviceMemory& memory);

    void create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);
    void destroy_buffer(VkBuffer buffer, VkDeviceMemory memory);

    void copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size);

    vk_rhi* get_rhi() const noexcept { return m_rhi; }

private:
    vk_rhi* m_rhi;
};

class vk_image : public vk_resource
{
public:
    vk_image(vk_rhi* rhi);
    virtual ~vk_image();

    VkImageView get_image_view() const noexcept { return m_image_view; }
    VkImageLayout get_image_layout() const noexcept { return m_image_layout; }

    virtual rhi_resource_format get_format() const noexcept override;
    virtual rhi_resource_extent get_extent() const noexcept override
    {
        return {m_extent.width, m_extent.height};
    }
    virtual std::size_t get_hash() const noexcept override { return m_hash; }

protected:
    void set(
        VkImage image,
        VkDeviceMemory memory,
        VkImageView image_view,
        VkImageLayout image_layout,
        VkFormat format,
        VkExtent2D extent,
        std::size_t hash);

    void create_image(
        std::uint32_t width,
        std::uint32_t height,
        VkFormat format,
        VkSampleCountFlagBits samples,
        VkImageUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& memory);
    void destroy_image(VkImage image, VkDeviceMemory memory);

    void create_image_view(VkImage image, VkFormat format, VkImageView& image_view);
    void destroy_image_view(VkImageView image_view);

    void copy_buffer_to_image(
        VkBuffer buffer,
        VkImage image,
        std::uint32_t width,
        std::uint32_t height);

    void transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout);

private:
    VkImage m_image;
    VkDeviceMemory m_memory;
    VkImageView m_image_view;
    VkImageLayout m_image_layout;

    VkFormat m_format;
    VkExtent2D m_extent;

    std::size_t m_hash;
};

class vk_swapchain_image : public vk_image
{
public:
    vk_swapchain_image(VkImage image, VkFormat format, const VkExtent2D& extent, vk_rhi* rhi);
    virtual ~vk_swapchain_image();
};

class vk_texture : public vk_image
{
public:
    vk_texture(const char* file, vk_rhi* rhi);
    virtual ~vk_texture();
};

class vk_sampler : public rhi_sampler
{
public:
    vk_sampler(const rhi_sampler_desc& desc, vk_rhi* rhi);
    vk_sampler(const vk_sampler&) = delete;
    virtual ~vk_sampler();

    VkSampler get_sampler() const noexcept { return m_sampler; }

    vk_sampler& operator=(const vk_sampler&) = delete;

private:
    VkSampler m_sampler;
    vk_rhi* m_rhi;
};

class vk_buffer : public vk_resource
{
public:
    vk_buffer(vk_rhi* rhi);

    virtual VkBuffer get_buffer_handle() const noexcept = 0;
};

class vk_vertex_buffer : public vk_buffer
{
public:
    vk_vertex_buffer(const rhi_vertex_buffer_desc& desc, vk_rhi* rhi);
    virtual ~vk_vertex_buffer();

    virtual void* get_buffer() override { return m_mapping_pointer; }
    virtual std::size_t get_buffer_size() const noexcept override { return m_buffer_size; }

    virtual std::size_t get_hash() const noexcept override
    {
        std::hash<void*> hasher;
        return hasher(m_buffer);
    }

    virtual VkBuffer get_buffer_handle() const noexcept override { return m_buffer; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    std::size_t m_buffer_size;

    void* m_mapping_pointer;
};

class vk_index_buffer : public vk_buffer
{
public:
    vk_index_buffer(const rhi_index_buffer_desc& desc, vk_rhi* rhi);
    virtual ~vk_index_buffer();

    virtual void* get_buffer() override { return m_mapping_pointer; }
    virtual std::size_t get_buffer_size() const noexcept override { return m_buffer_size; }

    virtual std::size_t get_hash() const noexcept override
    {
        std::hash<void*> hasher;
        return hasher(m_buffer);
    }

    virtual VkBuffer get_buffer_handle() const noexcept override { return m_buffer; }
    VkIndexType get_index_type() const noexcept { return m_index_type; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    std::size_t m_buffer_size;

    void* m_mapping_pointer;

    VkIndexType m_index_type;
};

class vk_uniform_buffer : public vk_buffer
{
public:
    vk_uniform_buffer(void* data, std::size_t size, vk_rhi* rhi);
    virtual ~vk_uniform_buffer();

    virtual void* get_buffer() override { return m_mapping_pointer; }
    virtual std::size_t get_buffer_size() const noexcept override { return m_buffer_size; }

    virtual std::size_t get_hash() const noexcept override
    {
        std::hash<void*> hasher;
        return hasher(m_buffer);
    }

    virtual VkBuffer get_buffer_handle() const noexcept override { return m_buffer; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    std::size_t m_buffer_size;

    void* m_mapping_pointer;
};
} // namespace violet::vk
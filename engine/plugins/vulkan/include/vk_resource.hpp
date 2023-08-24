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
    vk_rhi* get_rhi() const noexcept { return m_rhi; }

private:
    vk_rhi* m_rhi;
};

class vk_image : public vk_resource
{
public:
    vk_image(vk_rhi* rhi);

    virtual VkImageView get_image_view() const noexcept = 0;

protected:
    void create_image_view(VkImageViewCreateInfo* info, VkImageView& image_view);
    void destroy_image_view(VkImageView image_view);
};

class vk_swapchain_image : public vk_image
{
public:
    vk_swapchain_image(VkImage image, VkFormat format, const VkExtent2D& extent, vk_rhi* rhi);
    virtual ~vk_swapchain_image();

    virtual VkImageView get_image_view() const noexcept override;
    virtual rhi_resource_format get_format() const noexcept override;
    virtual rhi_resource_extent get_extent() const noexcept override;

    virtual std::size_t get_hash() const noexcept override;

protected:
    VkImage m_image;
    VkImageView m_image_view;

    VkFormat m_format;
    VkExtent2D m_extent;
};

class vk_buffer : public vk_resource
{
public:
    vk_buffer(vk_rhi* rhi);

    virtual VkBuffer get_buffer_handle() const noexcept = 0;

protected:
    void create_buffer(
        VkDeviceSize size,
        VkBufferUsageFlags usage,
        VkMemoryPropertyFlags properties,
        VkBuffer& buffer,
        VkDeviceMemory& memory);
    void destroy_buffer(VkBuffer buffer, VkDeviceMemory memory);

    void copy_buffer(VkBuffer source, VkBuffer target, VkDeviceSize size);
};

class vk_vertex_buffer : public vk_buffer
{
public:
    vk_vertex_buffer(const rhi_vertex_buffer_desc& desc, vk_rhi* rhi);
    virtual ~vk_vertex_buffer();

    virtual void* get_buffer() override { return m_mapping_pointer; }
    virtual std::size_t get_buffer_size() const noexcept override { return m_buffer_size; }

    virtual std::size_t get_hash() const noexcept override;

    virtual VkBuffer get_buffer_handle() const noexcept override { return m_buffer; }

private:
    VkBuffer m_buffer;
    VkDeviceMemory m_memory;
    std::size_t m_buffer_size;

    void* m_mapping_pointer;
};
} // namespace violet::vk
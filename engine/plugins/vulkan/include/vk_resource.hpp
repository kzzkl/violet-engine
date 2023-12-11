#pragma once

#include "vk_context.hpp"
#include <vector>

namespace violet::vk
{
class vk_resource : public rhi_resource
{
public:
    vk_resource(vk_context* context);
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

    vk_context* get_context() const noexcept { return m_context; }

private:
    vk_context* m_context;
};

class vk_image : public vk_resource
{
public:
    vk_image(vk_context* context);
    virtual ~vk_image();

    VkImageView get_image_view() const noexcept { return m_image_view; }
    VkImageLayout get_image_layout() const noexcept { return m_image_layout; }
    VkClearValue get_clear_value() const noexcept { return m_clear_value; }

    virtual rhi_resource_format get_format() const noexcept override;
    virtual rhi_resource_extent get_extent() const noexcept override
    {
        return {m_extent.width, m_extent.height};
    }
    virtual std::size_t get_hash() const noexcept override { return m_hash; }

protected:
    void set_image(VkImage image, VkDeviceMemory memory) noexcept
    {
        m_image = image;
        m_memory = memory;
    }
    void set_image_view(VkImageView image_view) noexcept { m_image_view = image_view; }
    void set_image_layout(VkImageLayout image_layout) noexcept { m_image_layout = image_layout; }
    void set_format(VkFormat format) noexcept { m_format = format; }
    void set_extent(VkExtent2D extent) noexcept { m_extent = extent; }

    void set_clear_value(VkClearColorValue clear_value) noexcept
    {
        m_clear_value.color = clear_value;
    }
    void set_clear_value(VkClearDepthStencilValue clear_value) noexcept
    {
        m_clear_value.depthStencil = clear_value;
    }

    void set_hash(std::size_t hash) noexcept { m_hash = hash; }

    void create_image(
        const VkImageCreateInfo& image_info,
        VkMemoryPropertyFlags properties,
        VkImage& image,
        VkDeviceMemory& memory);
    void destroy_image(VkImage image, VkDeviceMemory memory);

    void create_image_view(
        VkImage image,
        VkFormat format,
        VkImageViewType view_type,
        VkImageAspectFlags aspect_mask,
        VkImageView& image_view);
    void destroy_image_view(VkImageView image_view);

    void copy_buffer_to_image(
        VkBuffer buffer,
        VkImage image,
        std::uint32_t width,
        std::uint32_t height);
    void copy_buffer_to_image(
        VkBuffer buffer,
        VkImage image,
        const std::vector<VkBufferImageCopy>& regions);

    void transition_image_layout(
        VkImage image,
        VkImageLayout old_layout,
        VkImageLayout new_layout,
        std::uint32_t layer_count = 1);

private:
    VkImage m_image;
    VkDeviceMemory m_memory;
    VkImageView m_image_view;
    VkImageLayout m_image_layout;

    VkFormat m_format;
    VkExtent2D m_extent;

    VkClearValue m_clear_value;

    std::size_t m_hash;
};

class vk_depth_stencil : public vk_image
{
public:
    vk_depth_stencil(const rhi_depth_stencil_buffer_desc& desc, vk_context* context);
    virtual ~vk_depth_stencil();
};

class vk_texture : public vk_image
{
public:
    vk_texture(const char* file, vk_context* context);
    virtual ~vk_texture();
};

class vk_texture_cube : public vk_image
{
public:
    vk_texture_cube(
        const char* right,
        const char* left,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back,
        vk_context* context);
    ~vk_texture_cube();
};

class vk_sampler : public rhi_sampler
{
public:
    vk_sampler(const rhi_sampler_desc& desc, vk_context* context);
    vk_sampler(const vk_sampler&) = delete;
    virtual ~vk_sampler();

    VkSampler get_sampler() const noexcept { return m_sampler; }

    vk_sampler& operator=(const vk_sampler&) = delete;

private:
    VkSampler m_sampler;
    vk_context* m_context;
};

class vk_buffer : public vk_resource
{
public:
    vk_buffer(vk_context* context);

    virtual VkBuffer get_buffer_handle() const noexcept = 0;
};

class vk_vertex_buffer : public vk_buffer
{
public:
    vk_vertex_buffer(const rhi_buffer_desc& desc, vk_context* context);
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
    vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context);
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
    vk_uniform_buffer(void* data, std::size_t size, vk_context* context);
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

class vk_storage_buffer : public vk_buffer
{
public:
    vk_storage_buffer(const rhi_buffer_desc& desc, vk_context* context);
    virtual ~vk_storage_buffer();

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
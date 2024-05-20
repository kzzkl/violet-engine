#pragma once

#include "vk_context.hpp"
#include <vector>

namespace violet::vk
{
class vk_image : public rhi_texture
{
public:
    vk_image(vk_context* context);
    vk_image(const rhi_texture_desc& desc, vk_context* context);
    virtual ~vk_image();

    VkImage get_image() const noexcept { return m_image; }
    VkImageView get_image_view() const noexcept { return m_image_view; }
    VkImageLayout get_image_layout() const noexcept { return m_image_layout; }
    VkClearValue get_clear_value() const noexcept { return m_clear_value; }

    virtual rhi_format get_format() const noexcept override;
    virtual rhi_texture_extent get_extent() const noexcept override
    {
        return {m_extent.width, m_extent.height};
    }
    virtual std::size_t get_hash() const noexcept override { return m_hash; }

protected:
    void set_image(VkImage image, VmaAllocation allocation) noexcept;
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

    vk_context* get_context() const noexcept { return m_context; }

private:
    VkImage m_image;
    VmaAllocation m_allocation;
    VkImageView m_image_view;
    VkImageLayout m_image_layout;

    VkFormat m_format;
    VkExtent2D m_extent;

    VkClearValue m_clear_value;

    std::size_t m_hash;

    vk_context* m_context;
};

class vk_texture : public vk_image
{
public:
    vk_texture(const char* file, const rhi_texture_desc& desc, vk_context* context);
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
        const rhi_texture_desc& desc,
        vk_context* context);
    vk_texture_cube(const rhi_texture_desc& desc, vk_context* context);
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

class vk_buffer : public rhi_buffer
{
public:
    vk_buffer(vk_context* context);
    virtual ~vk_buffer();

    void* get_buffer() override { return m_mapping_pointer; }
    std::size_t get_buffer_size() const noexcept override { return m_buffer_size; }

    std::size_t get_hash() const noexcept override;

    VkBuffer get_buffer_handle() const noexcept { return m_buffer; }

protected:
    void set_buffer(VkBuffer buffer, VmaAllocation allocation, std::size_t buffer_size) noexcept
    {
        m_buffer = buffer;
        m_allocation = allocation;
        m_buffer_size = buffer_size;
    }

    void set_mapping_pointer(void* mapping_pointer) noexcept
    {
        m_mapping_pointer = mapping_pointer;
    }

    vk_context* get_context() const noexcept { return m_context; }

private:
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    std::size_t m_buffer_size;

    void* m_mapping_pointer;

    vk_context* m_context;
};

class vk_vertex_buffer : public vk_buffer
{
public:
    vk_vertex_buffer(const rhi_buffer_desc& desc, vk_context* context);
};

class vk_index_buffer : public vk_buffer
{
public:
    vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context);

    VkIndexType get_index_type() const noexcept { return m_index_type; }

private:
    VkIndexType m_index_type;
};

class vk_uniform_buffer : public vk_buffer
{
public:
    vk_uniform_buffer(void* data, std::size_t size, vk_context* context);
};

class vk_storage_buffer : public vk_buffer
{
public:
    vk_storage_buffer(const rhi_buffer_desc& desc, vk_context* context);
};
} // namespace violet::vk
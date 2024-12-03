#pragma once

#include "vk_common.hpp"

namespace violet::vk
{
class vk_context;

class vk_image : public rhi_texture
{
public:
    vk_image();
    vk_image(const vk_image&) = delete;
    virtual ~vk_image() = default;

    vk_image& operator=(const vk_image&) = delete;

    virtual VkImage get_image() const noexcept = 0;
    virtual VkImageView get_image_view() const noexcept = 0;
    virtual VkClearValue get_clear_value() const noexcept = 0;
    virtual VkImageAspectFlags get_aspect_mask() const noexcept = 0;
};

class vk_texture : public vk_image
{
public:
    vk_texture() = default;
    vk_texture(const rhi_texture_desc& desc, vk_context* context);
    virtual ~vk_texture();

    void set_handle(rhi_resource_handle handle) noexcept
    {
        m_handle = handle;
    }

    rhi_resource_handle get_handle() const noexcept override
    {
        return m_handle;
    }

    rhi_format get_format() const noexcept override
    {
        return m_format;
    }

    rhi_sample_count get_samples() const noexcept override
    {
        return m_samples;
    }

    rhi_texture_extent get_extent() const noexcept override
    {
        return m_extent;
    }

    std::uint32_t get_level() const noexcept override
    {
        return 0;
    }

    std::uint32_t get_level_count() const noexcept override
    {
        return m_level_count;
    }

    std::uint32_t get_layer() const noexcept override
    {
        return 0;
    }

    std::uint32_t get_layer_count() const noexcept override
    {
        return m_layer_count;
    }

    VkImage get_image() const noexcept override
    {
        return m_image;
    }

    VkImageView get_image_view() const noexcept override
    {
        return m_image_view;
    }

    VkClearValue get_clear_value() const noexcept override
    {
        return m_clear_value;
    }

    VkImageAspectFlags get_aspect_mask() const noexcept override
    {
        return m_aspect_mask;
    }

protected:
    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_image_view{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};

    rhi_format m_format{RHI_FORMAT_UNDEFINED};
    rhi_sample_count m_samples{RHI_SAMPLE_COUNT_1};
    rhi_texture_extent m_extent;
    std::uint32_t m_level_count{0};
    std::uint32_t m_layer_count{0};

    VkClearValue m_clear_value;
    VkImageAspectFlags m_aspect_mask;

    rhi_resource_handle m_handle{RHI_INVALID_RESOURCE_HANDLE};

    vk_context* m_context{nullptr};
};

class vk_texture_view : public vk_image
{
public:
    vk_texture_view(const rhi_texture_view_desc& desc, vk_context* context);
    virtual ~vk_texture_view();

    void set_handle(rhi_resource_handle handle) noexcept
    {
        m_handle = handle;
    }

    rhi_resource_handle get_handle() const noexcept override
    {
        return m_handle;
    }

    VkImage get_image() const noexcept override
    {
        return m_texture->get_image();
    }

    VkImageView get_image_view() const noexcept override
    {
        return m_image_view;
    }

    VkClearValue get_clear_value() const noexcept override
    {
        return m_texture->get_clear_value();
    }

    VkImageAspectFlags get_aspect_mask() const noexcept override
    {
        return m_texture->get_aspect_mask();
    }

    rhi_format get_format() const noexcept override
    {
        return m_texture->get_format();
    }

    rhi_sample_count get_samples() const noexcept override
    {
        return m_texture->get_samples();
    }

    rhi_texture_extent get_extent() const noexcept override
    {
        rhi_texture_extent extent = m_texture->get_extent();
        std::uint32_t width = (std::max)(1u, extent.width >> m_level);
        std::uint32_t height = (std::max)(1u, extent.height >> m_level);
        return {width, height};
    }

    std::uint32_t get_level() const noexcept override
    {
        return m_level;
    }

    std::uint32_t get_level_count() const noexcept override
    {
        return m_level_count;
    }

    std::uint32_t get_layer() const noexcept override
    {
        return m_layer;
    }

    std::uint32_t get_layer_count() const noexcept override
    {
        return m_layer_count;
    }

private:
    vk_texture* m_texture{nullptr};
    VkImageView m_image_view{VK_NULL_HANDLE};

    std::uint32_t m_level{0};
    std::uint32_t m_level_count{0};
    std::uint32_t m_layer{0};
    std::uint32_t m_layer_count{0};

    rhi_resource_handle m_handle{RHI_INVALID_RESOURCE_HANDLE};

    vk_context* m_context{nullptr};
};

class vk_sampler : public rhi_sampler
{
public:
    vk_sampler(const rhi_sampler_desc& desc, vk_context* context);
    vk_sampler(const vk_sampler&) = delete;
    virtual ~vk_sampler();

    void set_handle(rhi_resource_handle handle) noexcept
    {
        m_handle = handle;
    }

    rhi_resource_handle get_handle() const noexcept override
    {
        return m_handle;
    }

    VkSampler get_sampler() const noexcept
    {
        return m_sampler;
    }

    vk_sampler& operator=(const vk_sampler&) = delete;

private:
    VkSampler m_sampler;

    rhi_resource_handle m_handle{RHI_INVALID_RESOURCE_HANDLE};

    vk_context* m_context;
};

class vk_buffer : public rhi_buffer
{
public:
    vk_buffer(const rhi_buffer_desc& desc, vk_context* context);
    virtual ~vk_buffer();

    void* get_buffer_pointer() const noexcept override
    {
        return m_mapping_pointer;
    }

    std::size_t get_buffer_size() const noexcept override
    {
        return m_buffer_size;
    }

    void set_handle(rhi_resource_handle handle) noexcept
    {
        m_handle = handle;
    }

    rhi_resource_handle get_handle() const noexcept override
    {
        return m_handle;
    }

    VkBuffer get_buffer() const noexcept
    {
        return m_buffer;
    }

    rhi_buffer_flags get_flags() const noexcept
    {
        return m_flags;
    }

protected:
    vk_context* get_context() const noexcept
    {
        return m_context;
    }

private:
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    std::size_t m_buffer_size;

    rhi_buffer_flags m_flags{0};

    void* m_mapping_pointer;

    rhi_resource_handle m_handle{RHI_INVALID_RESOURCE_HANDLE};

    vk_context* m_context;
};

class vk_index_buffer : public vk_buffer
{
public:
    vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context);

    VkIndexType get_index_type() const noexcept
    {
        return m_index_type;
    }

private:
    VkIndexType m_index_type;
};

class vk_texel_buffer : public vk_buffer
{
public:
    vk_texel_buffer(const rhi_buffer_desc& desc, vk_context* context);
    virtual ~vk_texel_buffer();

    VkBufferView get_buffer_view() const noexcept
    {
        return m_buffer_view;
    }

private:
    VkBufferView m_buffer_view{VK_NULL_HANDLE};
};
} // namespace violet::vk
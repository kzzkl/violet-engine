#pragma once

#include "vk_context.hpp"
#include <vector>

namespace violet::vk
{
class vk_image : public rhi_texture
{
public:
    vk_image() = default;
    vk_image(const vk_image&) = delete;
    virtual ~vk_image() = default;

    virtual VkImage get_image() const noexcept = 0;
    virtual VkImageView get_image_view() const noexcept = 0;
    virtual VkClearValue get_clear_value() const noexcept = 0;

    virtual bool is_texture_view() const noexcept = 0;

    vk_image& operator=(const vk_image&) = delete;
};

class vk_texture : public vk_image
{
public:
    vk_texture() = default;
    vk_texture(const rhi_texture_desc& desc, vk_context* context);
    virtual ~vk_texture();

    virtual VkImage get_image() const noexcept override { return m_image; }
    virtual VkImageView get_image_view() const noexcept override { return m_image_view; }
    virtual VkClearValue get_clear_value() const noexcept override { return m_clear_value; }

    virtual rhi_format get_format() const noexcept override { return m_format; }
    virtual rhi_sample_count get_samples() const noexcept override { return m_samples; }
    virtual rhi_texture_extent get_extent() const noexcept override { return m_extent; }
    virtual std::uint32_t get_level_count() const noexcept override { return m_level_count; }
    virtual std::uint32_t get_layer_count() const noexcept override { return m_layer_count; }
    virtual std::uint64_t get_hash() const noexcept override { return m_hash; }

    virtual bool is_texture_view() const noexcept final { return false; }

    bool is_cube() const noexcept { return m_flags & RHI_TEXTURE_CUBE; }
    bool is_depth() const noexcept { return m_flags & RHI_TEXTURE_DEPTH_STENCIL; }

protected:
    void create_from_info(const rhi_texture_desc& desc);
    void create_from_file(const rhi_texture_desc& desc);
    static VkImageUsageFlags get_usage_flags(rhi_texture_flags flags);

    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_image_view{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};

    rhi_format m_format{RHI_FORMAT_UNDEFINED};
    rhi_sample_count m_samples{RHI_SAMPLE_COUNT_1};
    rhi_texture_extent m_extent;
    rhi_texture_flags m_flags{0};
    std::uint32_t m_level_count{0};
    std::uint32_t m_layer_count{0};

    VkClearValue m_clear_value;
    std::uint64_t m_hash{0};

    vk_context* m_context{nullptr};
};

class vk_texture_view : public vk_image
{
public:
    vk_texture_view(const rhi_texture_view_desc& desc, vk_context* context);
    virtual ~vk_texture_view();

    virtual VkImage get_image() const noexcept override { return m_texture->get_image(); }
    virtual VkImageView get_image_view() const noexcept override { return m_image_view; }
    virtual VkClearValue get_clear_value() const noexcept override
    {
        return m_texture->get_clear_value();
    }

    virtual rhi_format get_format() const noexcept override { return m_texture->get_format(); }
    virtual rhi_sample_count get_samples() const noexcept override
    {
        return m_texture->get_samples();
    }
    virtual rhi_texture_extent get_extent() const noexcept override
    {
        rhi_texture_extent extent = m_texture->get_extent();
        std::uint32_t width = (std::max)(1u, extent.width >> m_level);
        std::uint32_t height = (std::max)(1u, extent.height >> m_level);
        return {width, height};
    }
    virtual std::uint32_t get_level_count() const noexcept override
    {
        return m_texture->get_level_count();
    }
    virtual std::uint32_t get_layer_count() const noexcept override
    {
        return m_texture->get_layer_count();
    }
    virtual std::uint64_t get_hash() const noexcept override { return m_hash; }

    virtual bool is_texture_view() const noexcept final { return true; }

private:
    vk_texture* m_texture{nullptr};
    VkImageView m_image_view{VK_NULL_HANDLE};
    std::uint32_t m_level;
    std::uint64_t m_hash{0};

    vk_context* m_context{nullptr};
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
    vk_buffer(const rhi_buffer_desc& desc, vk_context* context);
    virtual ~vk_buffer();

    void* get_buffer() override { return m_mapping_pointer; }
    std::size_t get_buffer_size() const noexcept override { return m_buffer_size; }

    std::uint64_t get_hash() const noexcept override;

    VkBuffer get_buffer_handle() const noexcept { return m_buffer; }

private:
    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    std::size_t m_buffer_size;

    void* m_mapping_pointer;

    vk_context* m_context;
};

class vk_index_buffer : public vk_buffer
{
public:
    vk_index_buffer(const rhi_buffer_desc& desc, vk_context* context);

    VkIndexType get_index_type() const noexcept { return m_index_type; }

private:
    VkIndexType m_index_type;
};
} // namespace violet::vk
#pragma once

#include "vk_context.hpp"
#include <vector>

namespace violet::vk
{
class vk_texture : public rhi_texture
{
public:
    vk_texture() = default;
    vk_texture(const rhi_texture_desc& desc, vk_context* context);
    vk_texture(const char* file, const rhi_texture_desc& desc, vk_context* context);
    vk_texture(
        const void* data,
        std::size_t size,
        const rhi_texture_desc& desc,
        vk_context* context);
    vk_texture(
        const char* right,
        const char* left,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back,
        const rhi_texture_desc& desc,
        vk_context* context);
    virtual ~vk_texture();

    VkImage get_image() const noexcept { return m_image; }
    VkImageView get_image_view() const noexcept { return m_image_view; }
    VkClearValue get_clear_value() const noexcept { return m_clear_value; }

    virtual rhi_format get_format() const noexcept override { return m_format; }
    virtual rhi_sample_count get_samples() const noexcept override { return m_samples; }
    virtual rhi_texture_extent get_extent() const noexcept override { return m_extent; }
    virtual std::uint64_t get_hash() const noexcept override { return m_hash; }

protected:
    VkImage m_image{VK_NULL_HANDLE};
    VkImageView m_image_view{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};

    rhi_format m_format;
    rhi_sample_count m_samples;
    rhi_texture_extent m_extent;

    VkClearValue m_clear_value;
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
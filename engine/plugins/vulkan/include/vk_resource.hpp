#pragma once

#include "common/hash.hpp"
#include "vk_common.hpp"
#include <memory>
#include <unordered_map>

namespace violet::vk
{
class vk_context;
class vk_texture;

class vk_descriptor
{
public:
    virtual ~vk_descriptor() = default;

    virtual void write(
        VkDescriptorSet descriptor_set,
        std::uint32_t binding,
        std::uint32_t array_element) const
    {
    }
};

class vk_texture_descriptor : public vk_descriptor
{
public:
    vk_texture_descriptor(vk_texture* texture, VkImageView image_view);

    VkImageView get_image_view() const noexcept
    {
        return m_image_view;
    }

    vk_texture* get_texture() const noexcept
    {
        return m_texture;
    }

private:
    VkImageView m_image_view{VK_NULL_HANDLE};
    vk_texture* m_texture{nullptr};
};

class vk_texture_srv : public rhi_texture_srv, public vk_texture_descriptor
{
public:
    vk_texture_srv(vk_texture* texture, VkImageView image_view, vk_context* context);
    virtual ~vk_texture_srv();

    std::uint32_t get_bindless() const noexcept override
    {
        return m_bindless;
    }

    void write(VkDescriptorSet descriptor_set, std::uint32_t binding, std::uint32_t array_element)
        const override;

private:
    vk_context* m_context;
    std::uint32_t m_bindless{RHI_INVALID_BINDLESS_HANDLE};
};

class vk_texture_uav : public rhi_texture_uav, public vk_texture_descriptor
{
public:
    vk_texture_uav(vk_texture* texture, VkImageView image_view, vk_context* context);
    virtual ~vk_texture_uav();

    std::uint32_t get_bindless() const noexcept override
    {
        return m_bindless;
    }

    void write(VkDescriptorSet descriptor_set, std::uint32_t binding, std::uint32_t array_element)
        const override;

private:
    vk_context* m_context;
    std::uint32_t m_bindless{RHI_INVALID_BINDLESS_HANDLE};
};

class vk_texture_rtv : public rhi_texture_rtv, public vk_texture_descriptor
{
public:
    vk_texture_rtv(vk_texture* texture, VkImageView image_view, vk_context* context);
    virtual ~vk_texture_rtv();

private:
    vk_context* m_context;
};

class vk_texture_dsv : public rhi_texture_dsv, public vk_texture_descriptor
{
public:
    vk_texture_dsv(vk_texture* texture, VkImageView image_view, vk_context* context);
    virtual ~vk_texture_dsv();

private:
    vk_context* m_context;
};

class vk_texture : public rhi_texture
{
public:
    vk_texture(const rhi_texture_desc& desc, vk_context* context);
    vk_texture(
        VkImage image,
        VkFormat format,
        VkExtent2D extent,
        rhi_texture_flags flags,
        vk_context* context);
    vk_texture(const vk_texture&) = delete;
    virtual ~vk_texture();

    vk_texture& operator=(const vk_texture&) = delete;

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

    std::uint32_t get_level_count() const noexcept override
    {
        return m_level_count;
    }

    std::uint32_t get_layer_count() const noexcept override
    {
        return m_layer_count;
    }

    rhi_texture_flags get_flags() const noexcept override
    {
        return m_flags;
    }

    rhi_texture_srv* get_srv(
        rhi_texture_dimension dimension,
        std::uint32_t level,
        std::uint32_t level_count,
        std::uint32_t layer,
        std::uint32_t layer_count) override;

    rhi_texture_uav* get_uav(
        rhi_texture_dimension dimension,
        std::uint32_t level,
        std::uint32_t level_count,
        std::uint32_t layer,
        std::uint32_t layer_count) override;

    rhi_texture_rtv* get_rtv(
        rhi_texture_dimension dimension,
        std::uint32_t level,
        std::uint32_t level_count,
        std::uint32_t layer,
        std::uint32_t layer_count) override;

    rhi_texture_dsv* get_dsv(
        rhi_texture_dimension dimension,
        std::uint32_t level,
        std::uint32_t level_count,
        std::uint32_t layer,
        std::uint32_t layer_count) override;

    VkImage get_image() const noexcept
    {
        return m_image;
    }

    VkImageAspectFlags get_aspect_mask() const noexcept
    {
        return m_flags & RHI_TEXTURE_DEPTH_STENCIL ?
                   VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT :
                   VK_IMAGE_ASPECT_COLOR_BIT;
    }

private:
    struct view
    {
        VkImageView image_view{VK_NULL_HANDLE};

        std::unique_ptr<vk_texture_srv> srv;
        std::unique_ptr<vk_texture_uav> uav;
        std::unique_ptr<vk_texture_rtv> rtv;
        std::unique_ptr<vk_texture_dsv> dsv;
    };

    struct view_key
    {
        rhi_texture_dimension dimension;
        std::uint32_t level;
        std::uint32_t level_count;
        std::uint32_t layer;
        std::uint32_t layer_count;

        bool operator==(const view_key& other) const noexcept = default;
    };

    struct view_key_hash
    {
        std::size_t operator()(const view_key& key) const noexcept
        {
            return hash::city_hash_64(&key, sizeof(view_key));
        }
    };

    view& get_or_create_view(
        rhi_texture_dimension dimension,
        std::uint32_t level,
        std::uint32_t level_count,
        std::uint32_t layer,
        std::uint32_t layer_count);

    VkImage m_image{VK_NULL_HANDLE};
    VmaAllocation m_allocation{VK_NULL_HANDLE};

    rhi_format m_format{RHI_FORMAT_UNDEFINED};
    rhi_sample_count m_samples{RHI_SAMPLE_COUNT_1};
    rhi_texture_extent m_extent;
    std::uint32_t m_level_count{0};
    std::uint32_t m_layer_count{0};

    rhi_texture_flags m_flags{0};

    std::unordered_map<view_key, view, view_key_hash> m_views;

    vk_context* m_context{nullptr};
};

class vk_buffer;
class vk_buffer_descriptor : public vk_descriptor
{
public:
    vk_buffer_descriptor(
        vk_buffer* buffer,
        std::size_t offset,
        std::size_t size,
        VkBufferView buffer_view);

    vk_buffer* get_buffer() const noexcept
    {
        return m_buffer;
    }

    std::size_t get_offset() const noexcept
    {
        return m_offset;
    }

    std::size_t get_size() const noexcept
    {
        return m_size;
    }

    VkBufferView get_buffer_view() const noexcept
    {
        return m_buffer_view;
    }

private:
    vk_buffer* m_buffer{nullptr};

    std::size_t m_offset;
    std::size_t m_size;

    VkBufferView m_buffer_view{VK_NULL_HANDLE};
};

class vk_buffer_srv : public rhi_buffer_srv, public vk_buffer_descriptor
{
public:
    vk_buffer_srv(
        vk_buffer* buffer,
        std::size_t offset,
        std::size_t size,
        VkBufferView buffer_view,
        vk_context* context);
    virtual ~vk_buffer_srv();

    std::uint32_t get_bindless() const noexcept override
    {
        return m_bindless;
    }

    void write(VkDescriptorSet descriptor_set, std::uint32_t binding, std::uint32_t array_element)
        const override;

private:
    std::uint32_t m_bindless{RHI_INVALID_BINDLESS_HANDLE};

    vk_context* m_context;
};

class vk_buffer_uav : public rhi_buffer_uav, public vk_buffer_descriptor
{
public:
    vk_buffer_uav(
        vk_buffer* buffer,
        std::size_t offset,
        std::size_t size,
        VkBufferView buffer_view,
        vk_context* context);
    virtual ~vk_buffer_uav();

    std::uint32_t get_bindless() const noexcept override
    {
        return m_bindless;
    }

    void write(VkDescriptorSet descriptor_set, std::uint32_t binding, std::uint32_t array_element)
        const override;

private:
    std::uint32_t m_bindless{RHI_INVALID_BINDLESS_HANDLE};

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

    rhi_buffer_srv* get_srv(std::size_t offset, std::size_t size, rhi_format format) override;
    rhi_buffer_uav* get_uav(std::size_t offset, std::size_t size, rhi_format format) override;

    VkBuffer get_buffer() const noexcept
    {
        return m_buffer;
    }

    rhi_buffer_flags get_flags() const noexcept
    {
        return m_flags;
    }

private:
    struct view
    {
        VkBufferView buffer_view{VK_NULL_HANDLE};

        std::unique_ptr<vk_buffer_srv> srv;
        std::unique_ptr<vk_buffer_uav> uav;
    };

    struct view_key
    {
        std::size_t offset;
        std::size_t size;
        rhi_format format;

        bool operator==(const view_key& other) const noexcept = default;
    };

    struct view_key_hash
    {
        std::size_t operator()(const view_key& key) const noexcept
        {
            return hash::city_hash_64(&key, sizeof(view_key));
        }
    };

    view& get_or_create_view(std::size_t offset, std::size_t size, rhi_format format);

    VkBuffer m_buffer;
    VmaAllocation m_allocation;
    std::size_t m_buffer_size;

    rhi_buffer_flags m_flags{0};

    void* m_mapping_pointer;

    std::unordered_map<view_key, view, view_key_hash> m_views;

    vk_context* m_context;
};

class vk_sampler : public rhi_sampler, public vk_descriptor
{
public:
    vk_sampler(const rhi_sampler_desc& desc, vk_context* context);
    virtual ~vk_sampler();

    std::uint32_t get_bindless() const noexcept override
    {
        return m_bindless;
    }

    void write(VkDescriptorSet descriptor_set, std::uint32_t binding, std::uint32_t array_element)
        const override;

    VkSampler get_sampler() const noexcept
    {
        return m_sampler;
    }

private:
    VkSampler m_sampler;
    std::uint32_t m_bindless{RHI_INVALID_BINDLESS_HANDLE};

    vk_context* m_context;
};
} // namespace violet::vk
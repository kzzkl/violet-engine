#pragma once

#include "vk_common.hpp"
#include "vk_resource.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace violet::vk
{
class vk_parameter_layout
{
public:
    struct parameter_binding
    {
        std::size_t index;
        VkDescriptorType type;

        union {
            struct
            {
                std::size_t size;
            } uniform_buffer;
        };
    };

public:
    vk_parameter_layout(const rhi_parameter_desc& desc, vk_context* context);
    ~vk_parameter_layout();

    const std::vector<parameter_binding>& get_parameter_bindings() const noexcept
    {
        return m_parameter_bindings;
    }

    VkDescriptorSetLayout get_layout() const noexcept { return m_layout; }

private:
    VkDescriptorSetLayout m_layout;
    std::vector<parameter_binding> m_parameter_bindings;

    vk_context* m_context;
};

class vk_pipeline_layout
{
public:
    vk_pipeline_layout(
        const rhi_parameter_desc* parameters,
        std::size_t parameter_count,
        vk_context* context);
    ~vk_pipeline_layout();

    VkPipelineLayout get_layout() const noexcept { return m_layout; }

private:
    VkPipelineLayout m_layout;
    vk_context* m_context;
};

class vk_layout_manager
{
public:
    vk_layout_manager(vk_context* context);
    ~vk_layout_manager();

    vk_parameter_layout* get_parameter_layout(const rhi_parameter_desc& desc);
    vk_pipeline_layout* get_pipeline_layout(
        const rhi_parameter_desc* parameters,
        std::size_t parameter_count);

private:
    std::size_t get_hash(const rhi_parameter_desc& desc);

    std::unordered_map<std::size_t, std::unique_ptr<vk_parameter_layout>> m_parameter_layouts;
    std::unordered_map<std::size_t, std::unique_ptr<vk_pipeline_layout>> m_pipeline_layouts;

    vk_context* m_context;
};

class vk_parameter : public rhi_parameter
{
public:
    vk_parameter(const rhi_parameter_desc& desc, vk_context* context);
    virtual ~vk_parameter();

    virtual void set_uniform(
        std::size_t index,
        const void* data,
        std::size_t size,
        std::size_t offset) override;
    virtual void set_texture(std::size_t index, rhi_texture* texture, rhi_sampler* sampler)
        override;
    virtual void set_storage(std::size_t index, rhi_buffer* storage_buffer) override;

    VkDescriptorSet get_descriptor_set() const noexcept;

    void sync();

private:
    struct frame_resource
    {
        VkDescriptorSet descriptor_set;
        std::vector<std::size_t> descriptor_update_count;

        std::size_t update_count;
    };

    void mark_dirty(std::size_t descriptor_index);

    vk_parameter_layout* m_layout;
    std::vector<std::unique_ptr<vk_buffer>> m_uniform_buffers;
    std::vector<std::pair<VkImageView, VkSampler>> m_images;
    std::vector<frame_resource> m_frame_resources;

    vk_context* m_context;
};

class vk_shader : public rhi_shader
{
public:
    vk_shader(const char* path, VkDevice device);
    ~vk_shader();

    VkShaderModule get_module() const noexcept { return m_module; }

private:
    VkShaderModule m_module;
    VkDevice m_device;
};

class vk_render_pipeline : public rhi_render_pipeline
{
public:
    vk_render_pipeline(
        const rhi_render_pipeline_desc& desc,
        VkExtent2D extent,
        vk_context* context);
    vk_render_pipeline(const vk_render_pipeline&) = delete;
    virtual ~vk_render_pipeline();

    VkPipeline get_pipeline() const noexcept { return m_pipeline; }
    VkPipelineLayout get_pipeline_layout() const noexcept
    {
        return m_pipeline_layout->get_layout();
    }

    vk_render_pipeline& operator=(const vk_render_pipeline&) = delete;

private:
    VkPipeline m_pipeline;
    vk_pipeline_layout* m_pipeline_layout;

    vk_context* m_context;
};

class vk_compute_pipeline : public rhi_compute_pipeline
{
public:
    vk_compute_pipeline(const rhi_compute_pipeline_desc& desc, vk_context* context);
    vk_compute_pipeline(const vk_render_pipeline&) = delete;
    virtual ~vk_compute_pipeline();

    VkPipeline get_pipeline() const noexcept { return m_pipeline; }
    VkPipelineLayout get_pipeline_layout() const noexcept
    {
        return m_pipeline_layout->get_layout();
    }

    vk_render_pipeline& operator=(const vk_render_pipeline&) = delete;

private:
    VkPipeline m_pipeline;
    vk_pipeline_layout* m_pipeline_layout;

    vk_context* m_context;
};
} // namespace violet::vk

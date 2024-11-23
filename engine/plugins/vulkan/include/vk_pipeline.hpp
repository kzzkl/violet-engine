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
        VkDescriptorType type;

        union
        {
            struct
            {
                std::size_t index;
                std::size_t size;
            } uniform;

            struct
            {
                std::size_t index;
            } texture;
        };
    };

public:
    vk_parameter_layout(const rhi_parameter_desc& desc, vk_context* context);
    ~vk_parameter_layout();

    const std::vector<parameter_binding>& get_parameter_bindings() const noexcept
    {
        return m_parameter_bindings;
    }

    VkDescriptorSetLayout get_layout() const noexcept
    {
        return m_layout;
    }

private:
    VkDescriptorSetLayout m_layout;
    std::vector<parameter_binding> m_parameter_bindings;

    vk_context* m_context;
};

class vk_pipeline_layout
{
public:
    vk_pipeline_layout(std::span<vk_parameter_layout*> parameter_layouts, vk_context* context);
    ~vk_pipeline_layout();

    VkPipelineLayout get_layout() const noexcept
    {
        return m_layout;
    }

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
    vk_pipeline_layout* get_pipeline_layout(std::span<vk_parameter_layout*> parameter_layouts);

private:
    std::unordered_map<std::uint64_t, std::unique_ptr<vk_parameter_layout>> m_parameter_layouts;
    std::unordered_map<std::uint64_t, std::unique_ptr<vk_pipeline_layout>> m_pipeline_layouts;

    vk_context* m_context;
};

class vk_parameter : public rhi_parameter
{
public:
    vk_parameter(const rhi_parameter_desc& desc, vk_context* context);
    virtual ~vk_parameter();

    void set_uniform(std::size_t index, const void* data, std::size_t size, std::size_t offset)
        override;
    void set_uniform(std::size_t index, rhi_buffer* uniform_buffer) override;
    void set_texture(std::size_t index, rhi_texture* texture, rhi_sampler* sampler) override;
    void set_storage(std::size_t index, rhi_buffer* storage_buffer) override;

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
    struct parameter
    {
        std::uint32_t space;
        vk_parameter_layout* layout;
    };

public:
    vk_shader(const rhi_shader_desc& desc, vk_context* context);
    virtual ~vk_shader();

    VkShaderModule get_module() const noexcept
    {
        return m_module;
    }

    const std::vector<parameter>& get_parameters() const noexcept
    {
        return m_parameters;
    }

private:
    VkShaderModule m_module;
    vk_context* m_context;

    std::vector<parameter> m_parameters;
};

class vk_vertex_shader : public vk_shader
{
public:
    struct vertex_attribute
    {
        std::string name;
        rhi_format format;
    };

public:
    vk_vertex_shader(const rhi_shader_desc& desc, vk_context* context);

    const std::vector<vertex_attribute>& get_vertex_attributes() const noexcept
    {
        return m_vertex_attributes;
    }

private:
    std::vector<vertex_attribute> m_vertex_attributes;
};

class vk_fragment_shader : public vk_shader
{
public:
    using vk_shader::vk_shader;
};

class vk_compute_shader : public vk_shader
{
public:
    using vk_shader::vk_shader;
};

class vk_render_pipeline : public rhi_render_pipeline
{
public:
    vk_render_pipeline(const rhi_render_pipeline_desc& desc, vk_context* context);
    vk_render_pipeline(const vk_render_pipeline&) = delete;
    virtual ~vk_render_pipeline();

    VkPipeline get_pipeline() const noexcept
    {
        return m_pipeline;
    }
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

    VkPipeline get_pipeline() const noexcept
    {
        return m_pipeline;
    }
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

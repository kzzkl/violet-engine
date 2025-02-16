#pragma once

#include "vk_parameter.hpp"

namespace violet::vk
{
class vk_shader : public rhi_shader
{
public:
    struct parameter
    {
        std::uint32_t space;
        vk_parameter_layout* layout;
    };

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

class vk_raster_pipeline : public rhi_raster_pipeline
{
public:
    vk_raster_pipeline(const rhi_raster_pipeline_desc& desc, vk_context* context);
    vk_raster_pipeline(const vk_raster_pipeline&) = delete;
    virtual ~vk_raster_pipeline();

    vk_raster_pipeline& operator=(const vk_raster_pipeline&) = delete;

    VkPipeline get_pipeline() const noexcept
    {
        return m_pipeline;
    }

    VkPipelineLayout get_pipeline_layout() const noexcept
    {
        return m_pipeline_layout;
    }

private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;

    vk_context* m_context;
};

class vk_compute_pipeline : public rhi_compute_pipeline
{
public:
    vk_compute_pipeline(const rhi_compute_pipeline_desc& desc, vk_context* context);
    vk_compute_pipeline(const vk_compute_pipeline&) = delete;
    virtual ~vk_compute_pipeline();

    vk_compute_pipeline& operator=(const vk_compute_pipeline&) = delete;

    VkPipeline get_pipeline() const noexcept
    {
        return m_pipeline;
    }

    VkPipelineLayout get_pipeline_layout() const noexcept
    {
        return m_pipeline_layout;
    }

private:
    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;

    vk_context* m_context;
};
} // namespace violet::vk

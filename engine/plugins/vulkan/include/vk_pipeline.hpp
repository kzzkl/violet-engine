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

    virtual VkShaderStageFlagBits get_stage() const noexcept = 0;

    virtual std::string_view get_entry_point() const noexcept = 0;

    VkShaderModule get_module() const noexcept
    {
        return m_module;
    }

    std::uint32_t get_push_constant_size() const noexcept
    {
        return m_push_constant_size;
    }

    const std::vector<parameter>& get_parameters() const noexcept
    {
        return m_parameters;
    }

private:
    VkShaderModule m_module{VK_NULL_HANDLE};

    std::uint32_t m_push_constant_size;
    std::vector<parameter> m_parameters;

    vk_context* m_context;
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

    VkShaderStageFlagBits get_stage() const noexcept override
    {
        return VK_SHADER_STAGE_VERTEX_BIT;
    }

    std::string_view get_entry_point() const noexcept override
    {
        return "vs_main";
    }

    const std::vector<vertex_attribute>& get_vertex_attributes() const noexcept
    {
        return m_vertex_attributes;
    }

private:
    std::vector<vertex_attribute> m_vertex_attributes;
};

class vk_geometry_shader : public vk_shader
{
public:
    using vk_shader::vk_shader;

    VkShaderStageFlagBits get_stage() const noexcept override
    {
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    }

    std::string_view get_entry_point() const noexcept override
    {
        return "gs_main";
    }
};

class vk_fragment_shader : public vk_shader
{
public:
    using vk_shader::vk_shader;

    VkShaderStageFlagBits get_stage() const noexcept override
    {
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    std::string_view get_entry_point() const noexcept override
    {
        return "fs_main";
    }
};

class vk_compute_shader : public vk_shader
{
public:
    using vk_shader::vk_shader;

    VkShaderStageFlagBits get_stage() const noexcept override
    {
        return VK_SHADER_STAGE_COMPUTE_BIT;
    }

    std::string_view get_entry_point() const noexcept override
    {
        return "cs_main";
    }
};

class vk_pipeline_layout;

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

    vk_pipeline_layout* get_pipeline_layout() const noexcept
    {
        return m_pipeline_layout;
    }

private:
    VkPipeline m_pipeline;
    vk_pipeline_layout* m_pipeline_layout{nullptr};

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

    vk_pipeline_layout* get_pipeline_layout() const noexcept
    {
        return m_pipeline_layout;
    }

private:
    VkPipeline m_pipeline{VK_NULL_HANDLE};
    vk_pipeline_layout* m_pipeline_layout{nullptr};

    vk_context* m_context{nullptr};
};
} // namespace violet::vk

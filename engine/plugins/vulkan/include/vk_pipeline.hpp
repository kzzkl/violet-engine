#pragma once

#include "vk_common.hpp"
#include <string>
#include <vector>

namespace violet::vk
{
class vk_rhi;
class vk_render_pass : public render_pass_interface
{
public:
    vk_render_pass(const render_pass_desc& desc, vk_rhi* rhi);
    vk_render_pass(const vk_render_pass&) = delete;
    virtual ~vk_render_pass();

    VkRenderPass get_render_pass() const noexcept { return m_render_pass; }

    vk_render_pass& operator=(const vk_render_pass&) = delete;

private:
    VkExtent2D m_extent;
    VkRenderPass m_render_pass;

    vk_rhi* m_rhi;
};

class vk_pipeline_parameter : public pipeline_parameter_interface
{
public:
    vk_pipeline_parameter(const pipeline_parameter_desc& desc);
    virtual ~vk_pipeline_parameter();

    virtual void set(std::size_t index, const void* data, size_t size) override;
    virtual void set(std::size_t index, resource_interface* texture) override;

private:

};

class vk_render_pipeline : public render_pipeline_interface
{
public:
    vk_render_pipeline(
        const render_pipeline_desc& desc,
        VkExtent2D extent,
        vk_rhi* rhi);
    vk_render_pipeline(const vk_render_pipeline&) = delete;
    virtual ~vk_render_pipeline();

    VkPipeline get_pipeline() const noexcept { return m_pipeline; }
    VkPipelineLayout get_pipeline_layout() const noexcept { return m_pipeline_layout; }

    vk_render_pipeline& operator=(const vk_render_pipeline&) = delete;

private:
    VkShaderModule load_shader(std::string_view path);

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;

    vk_rhi* m_rhi;
};
} // namespace violet::vk

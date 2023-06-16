#pragma once

#include "vk_common.hpp"
#include <string>
#include <vector>

namespace violet::vk
{
class vk_pipeline_parameter : public rhi_pipeline_parameter
{
public:
    vk_pipeline_parameter(const rhi_pipeline_parameter_desc& desc);
    virtual ~vk_pipeline_parameter();

    virtual void set(std::size_t index, const void* data, size_t size) override;
    virtual void set(std::size_t index, rhi_resource* texture) override;

private:
};

class vk_rhi;
class vk_render_pipeline : public rhi_render_pipeline
{
public:
    vk_render_pipeline(const rhi_render_pipeline_desc& desc, VkExtent2D extent, vk_rhi* rhi);
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

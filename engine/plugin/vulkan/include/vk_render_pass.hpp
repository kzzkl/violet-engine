#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_render_pipeline : public render_pipeline
{
public:
    vk_render_pipeline(
        const render_pipeline_desc& desc,
        VkRenderPass render_pass,
        std::size_t index);

    void begin(VkCommandBuffer command_buffer);
    void end(VkCommandBuffer command_buffer);

    VkPipelineLayout layout() const noexcept { return m_layout; }
    VkPipeline pipeline() const noexcept { return m_pipeline; }

private:
    VkShaderModule load_shader(std::string_view file);

    VkPipelineLayout m_layout;
    VkPipeline m_pipeline;
};

class vk_render_pass : public render_pass
{
public:
    vk_render_pass(const render_pass_desc& desc);
    ~vk_render_pass();

    virtual render_pipeline* subpass(std::size_t index) override;
    
    void begin(VkCommandBuffer command_buffer, VkFramebuffer frame_buffer);
    void end(VkCommandBuffer command_buffer);

    VkRenderPass render_pass() const noexcept { return m_render_pass; }

private:
    void create_pass(const render_pass_desc& desc);

    VkRenderPass m_render_pass;
    std::vector<vk_render_pipeline> m_pipelines;
};
} // namespace ash::graphics::vk
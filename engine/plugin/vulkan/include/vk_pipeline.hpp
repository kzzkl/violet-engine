#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_pipeline
{
public:
    vk_pipeline(const pipeline_desc& desc);
    ~vk_pipeline();

    VkPipeline pipeline() const noexcept { return m_pipeline; }
    VkRenderPass pass() const noexcept { return m_pass; }
    VkFramebuffer frame_buffer(std::uint32_t index) const { return m_frame_buffers[index]; }

private:
    VkShaderModule load_shader(std::string_view file);

    VkPipelineLayout m_layout;
    VkRenderPass m_pass;

    VkPipeline m_pipeline;
    std::vector<VkFramebuffer> m_frame_buffers;
};

class vk_pass
{
public:
    vk_pass();

private:
};
} // namespace ash::graphics::vk
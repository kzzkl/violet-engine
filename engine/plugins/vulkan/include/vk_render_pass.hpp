#pragma once

#include "vk_context.hpp"

namespace violet::vk
{
class vk_render_pass : public rhi_render_pass
{
public:
    vk_render_pass(const rhi_render_pass_desc& desc, vk_context* context);
    vk_render_pass(const vk_render_pass&) = delete;
    virtual ~vk_render_pass();

    VkRenderPass get_render_pass() const noexcept { return m_render_pass; }

    vk_render_pass& operator=(const vk_render_pass&) = delete;

private:
    VkRenderPass m_render_pass;

    vk_context* m_context;
};
} // namespace violet::vk
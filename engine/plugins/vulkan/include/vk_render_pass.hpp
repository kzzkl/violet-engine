#pragma once

#include "vk_context.hpp"
#include <vector>

namespace violet::vk
{
class vk_render_pass : public rhi_render_pass
{
public:
    struct subpass_info
    {
        std::size_t render_target_count;
    };

    vk_render_pass(const rhi_render_pass_desc& desc, vk_context* context);
    vk_render_pass(const vk_render_pass&) = delete;
    virtual ~vk_render_pass();

    VkRenderPass get_render_pass() const noexcept { return m_render_pass; }

    const subpass_info& get_subpass_info(std::size_t subpass) const
    {
        return m_subpass_infos[subpass];
    }

    vk_render_pass& operator=(const vk_render_pass&) = delete;

private:
    VkRenderPass m_render_pass;
    std::vector<subpass_info> m_subpass_infos;

    vk_context* m_context;
};
} // namespace violet::vk
#pragma once

#include "vk_common.hpp"
#include "vk_context.hpp"
#include <vector>

namespace violet::vk
{
class vk_framebuffer : public rhi_framebuffer
{
public:
    vk_framebuffer(const rhi_framebuffer_desc& desc, vk_context* context);
    virtual ~vk_framebuffer();

    VkFramebuffer get_framebuffer() const noexcept { return m_framebuffer; }
    rhi_texture_extent get_extent() const noexcept { return m_extent; }

    const std::vector<VkClearValue>& get_clear_values() const noexcept { return m_clear_values; }

private:
    VkFramebuffer m_framebuffer;
    rhi_texture_extent m_extent;

    std::vector<VkClearValue> m_clear_values;

    vk_context* m_context;
};
} // namespace violet::vk
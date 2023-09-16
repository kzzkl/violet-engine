#pragma once

#include "vk_common.hpp"
#include <vector>

namespace violet::vk
{
class vk_rhi;
class vk_framebuffer : public rhi_framebuffer
{
public:
    vk_framebuffer(const rhi_framebuffer_desc& desc, vk_rhi* rhi);
    virtual ~vk_framebuffer();

    VkFramebuffer get_framebuffer() const noexcept { return m_framebuffer; }
    rhi_resource_extent get_extent() const noexcept { return m_extent; }

    const std::vector<VkClearValue>& get_clear_values() const noexcept { return m_clear_values; }

private:
    VkFramebuffer m_framebuffer;
    rhi_resource_extent m_extent;

    std::vector<VkClearValue> m_clear_values;

    vk_rhi* m_rhi;
};
} // namespace violet::vk
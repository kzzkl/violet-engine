#pragma once

#include "vk_common.hpp"

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

private:
    VkFramebuffer m_framebuffer;
    rhi_resource_extent m_extent;

    vk_rhi* m_rhi;
};
} // namespace violet::vk
#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_frame_buffer : public render_target_set_interface
{
public:
    vk_frame_buffer(const render_target_set_desc& desc);

    VkFramebuffer frame_buffer() const noexcept { return m_frame_buffer; }

private:
    VkFramebuffer m_frame_buffer;
};
} // namespace ash::graphics::vk
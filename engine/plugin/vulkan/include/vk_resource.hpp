#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_resource : public resource
{
public:
    vk_resource(VkImage image, VkImageView view) : m_image(image), m_view(view) {}

    VkImageView view() const noexcept { return m_view; }

private:
    VkImage m_image;
    VkImageView m_view;
};
} // namespace ash::graphics::vk
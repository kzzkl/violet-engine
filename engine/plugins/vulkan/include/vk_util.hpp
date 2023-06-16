#pragma once

#include "vk_common.hpp"

namespace violet::vk
{
class vk_util
{
public:
    static VkFormat map_format(rhi_resource_format format);
    static rhi_resource_format map_format(VkFormat format);

    static VkImageLayout map_state(rhi_resource_state state);
};
} // namespace violet::vk
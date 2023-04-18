#pragma once

#include "vk_common.hpp"

namespace violet::vk
{
VkFormat convert(resource_format format);
resource_format convert(VkFormat format);

VkImageLayout convert(resource_state state);
}
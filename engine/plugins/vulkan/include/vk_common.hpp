#pragma once

#include "graphics/render_interface.hpp"
#include "volk.h"

#include "vk_mem_alloc.h"
#include <stdexcept>

namespace violet::vk
{
inline void vk_check(VkResult result)
{
    if (result != VK_SUCCESS)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "RESULT of 0x%08X", result);

        throw std::runtime_error(s_str);
    }
}
} // namespace violet::vk
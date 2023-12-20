#pragma once

#include "graphics/render_interface.hpp"
#include <stdexcept>

#include "volk.h"

#ifndef NDEBUG
#    include <cassert>
#    define VIOLET_VK_ASSERT(condition, ...) assert(condition)
#else
#    define VIOLET_VK_ASSERT(condition, ...)
#endif

namespace violet::vk
{
class vk_exception : public std::runtime_error
{
public:
    vk_exception(std::string_view name) : std::runtime_error(name.data()), m_result(VK_SUCCESS) {}

    vk_exception(VkResult result) : std::runtime_error(result_to_string(result)), m_result(result)
    {
    }

    VkResult error() const { return m_result; }

private:
    std::string result_to_string(VkResult result)
    {
        char s_str[64] = {};
        sprintf_s(s_str, "RESULT of 0x%08X", m_result);
        return std::string(s_str);
    }

    VkResult m_result;
};

inline void vk_check(VkResult result)
{
    if (result != VK_SUCCESS)
        throw vk_exception(result);
}
} // namespace violet::vk
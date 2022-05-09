#pragma once

#include "graphics_interface.hpp"

#ifdef WIN32
#    include <Windows.h>
#    define ASH_VK_WIN32
#    define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.hpp>

#ifndef NDEBUG
#    include <cassert>
#    define ASH_VK_ASSERT(condition, ...) assert(condition)
#else
#    define ASH_VK_ASSERT(condition, ...)
#endif

namespace ash::graphics::vk
{
class vk_exception : public std::runtime_error
{
public:
    vk_exception(std::string_view name) : std::runtime_error(name.data()) {}

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

inline void throw_if_failed(VkResult result)
{
    if (result != VK_SUCCESS)
        throw vk_exception(result);
}

VkSampleCountFlagBits to_vk_samples(std::size_t samples);

VkAttachmentLoadOp to_vk_attachment_load_op(attachment_load_op op);
VkAttachmentStoreOp to_vk_attachment_store_op(attachment_store_op op);

VkFormat to_vk_format(resource_format format);
resource_format to_ash_format(VkFormat format);

VkImageLayout to_vk_image_layout(resource_state state);
} // namespace ash::graphics::vk
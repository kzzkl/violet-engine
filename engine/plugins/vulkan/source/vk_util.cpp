#include "vk_util.hpp"

namespace violet::vk
{
VkFormat convert(resource_format format)
{
    switch (format)
    {
    case RESOURCE_FORMAT_UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case RESOURCE_FORMAT_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case RESOURCE_FORMAT_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case RESOURCE_FORMAT_R32G32B32A32_FLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RESOURCE_FORMAT_R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case RESOURCE_FORMAT_R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case RESOURCE_FORMAT_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

resource_format convert(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return RESOURCE_FORMAT_UNDEFINED;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return RESOURCE_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return RESOURCE_FORMAT_B8G8R8A8_UNORM;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return RESOURCE_FORMAT_R32G32B32A32_FLOAT;
    case VK_FORMAT_R32G32B32A32_SINT:
        return RESOURCE_FORMAT_R32G32B32A32_SINT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return RESOURCE_FORMAT_R32G32B32A32_UINT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    default:
        return RESOURCE_FORMAT_UNDEFINED;
    }
}

VkImageLayout convert(resource_state state)
{
    switch (state)
    {
    case RESOURCE_STATE_UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case RESOURCE_STATE_SHADER_RESOURCE:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case RESOURCE_STATE_RENDER_TARGET:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RESOURCE_STATE_PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}
} // namespace violet::vk
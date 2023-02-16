#include "vk_common.hpp"

namespace violet::graphics::vk
{
VkSampleCountFlagBits to_vk_samples(std::size_t samples)
{
    switch (samples)
    {
    case 0:
    case 1:
        return VK_SAMPLE_COUNT_1_BIT;
    case 2:
        return VK_SAMPLE_COUNT_2_BIT;
    case 4:
        return VK_SAMPLE_COUNT_4_BIT;
    case 8:
        return VK_SAMPLE_COUNT_8_BIT;
    default:
        throw vk_exception("Invalid sample count.");
    }
}

VkAttachmentLoadOp to_vk_attachment_load_op(attachment_load_op op)
{
    switch (op)
    {
    case ATTACHMENT_LOAD_OP_LOAD:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case ATTACHMENT_LOAD_OP_CLEAR:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case ATTACHMENT_LOAD_OP_DONT_CARE:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        throw vk_exception("Invalid load op.");
    }
}

VkAttachmentStoreOp to_vk_attachment_store_op(attachment_store_op op)
{
    switch (op)
    {
    case ATTACHMENT_STORE_OP_STORE:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case ATTACHMENT_STORE_OP_DONT_CARE:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        throw vk_exception("Invalid store op.");
    }
}

VkFormat to_vk_format(resource_format format)
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
    case RESOURCE_FORMAT_R32G32B32A32_INT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case RESOURCE_FORMAT_R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case RESOURCE_FORMAT_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    default:
        throw vk_exception("Invalid resource format.");
    }
}

resource_format to_violet_format(VkFormat format)
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
        return RESOURCE_FORMAT_R32G32B32A32_INT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return RESOURCE_FORMAT_R32G32B32A32_UINT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    default:
        throw vk_exception("Invalid resource format.");
    }
}

VkImageLayout to_vk_image_layout(resource_state state)
{
    switch (state)
    {
    case RESOURCE_STATE_UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case RESOURCE_STATE_RENDER_TARGET:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RESOURCE_STATE_DEPTH_STENCIL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case RESOURCE_STATE_PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        throw vk_exception("Invalid resource state.");
    }
}
} // namespace violet::graphics::vk
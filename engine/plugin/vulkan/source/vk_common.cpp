#include "vk_common.hpp"

namespace ash::graphics::vk
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
    case attachment_load_op::LOAD:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    case attachment_load_op::CLEAR:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case attachment_load_op::DONT_CARE:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    default:
        throw vk_exception("Invalid load op.");
    }
}

VkAttachmentStoreOp to_vk_attachment_store_op(attachment_store_op op)
{
    switch (op)
    {
    case attachment_store_op::STORE:
        return VK_ATTACHMENT_STORE_OP_STORE;
    case attachment_store_op::DONT_CARE:
        return VK_ATTACHMENT_STORE_OP_DONT_CARE;
    default:
        throw vk_exception("Invalid store op.");
    }
}

VkFormat to_vk_format(resource_format format)
{
    switch (format)
    {
    case resource_format::UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case resource_format::R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case resource_format::R32G32B32A32_FLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case resource_format::R32G32B32A32_INT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case resource_format::R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case resource_format::D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    default:
        throw vk_exception("Invalid resource format.");
    }
}

resource_format to_ash_format(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_UNDEFINED:
        return resource_format::UNDEFINED;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return resource_format::R8G8B8A8_UNORM;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return resource_format::R32G32B32A32_FLOAT;
    case VK_FORMAT_R32G32B32A32_SINT:
        return resource_format::R32G32B32A32_INT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return resource_format::R32G32B32A32_UINT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return resource_format::D24_UNORM_S8_UINT;
    default:
        throw vk_exception("Invalid resource format.");
    }
}

VkImageLayout to_vk_image_layout(resource_state state)
{
    switch (state)
    {
    case resource_state::UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case resource_state::PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case resource_state::DEPTH_STENCIL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    default:
        throw vk_exception("Invalid resource state.");
    }
}
} // namespace ash::graphics::vk
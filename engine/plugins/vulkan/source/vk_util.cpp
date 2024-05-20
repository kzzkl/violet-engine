#include "vk_util.hpp"

namespace violet::vk
{
VkFormat vk_util::map_format(rhi_format format)
{
    switch (format)
    {
    case RHI_FORMAT_UNDEFINED:
        return VK_FORMAT_UNDEFINED;
    case RHI_FORMAT_R8_UNORM:
        return VK_FORMAT_R8_UNORM;
    case RHI_FORMAT_R8_SNORM:
        return VK_FORMAT_R8_SNORM;
    case RHI_FORMAT_R8_UINT:
        return VK_FORMAT_R8_UINT;
    case RHI_FORMAT_R8_SINT:
        return VK_FORMAT_R8_SINT;
    case RHI_FORMAT_R8G8_UNORM:
        return VK_FORMAT_R8G8_UNORM;
    case RHI_FORMAT_R8G8_SNORM:
        return VK_FORMAT_R8G8_SNORM;
    case RHI_FORMAT_R8G8_UINT:
        return VK_FORMAT_R8G8_UINT;
    case RHI_FORMAT_R8G8_SINT:
        return VK_FORMAT_R8G8_SINT;
    case RHI_FORMAT_R8G8B8_UNORM:
        return VK_FORMAT_R8G8B8_UNORM;
    case RHI_FORMAT_R8G8B8_SNORM:
        return VK_FORMAT_R8G8B8_SNORM;
    case RHI_FORMAT_R8G8B8_UINT:
        return VK_FORMAT_R8G8B8_UINT;
    case RHI_FORMAT_R8G8B8_SINT:
        return VK_FORMAT_R8G8B8_SINT;
    case RHI_FORMAT_R8G8B8A8_UNORM:
        return VK_FORMAT_R8G8B8A8_UNORM;
    case RHI_FORMAT_R8G8B8A8_SNORM:
        return VK_FORMAT_R8G8B8A8_SNORM;
    case RHI_FORMAT_R8G8B8A8_UINT:
        return VK_FORMAT_R8G8B8A8_UINT;
    case RHI_FORMAT_R8G8B8A8_SINT:
        return VK_FORMAT_R8G8B8A8_SINT;
    case RHI_FORMAT_B8G8R8A8_UNORM:
        return VK_FORMAT_B8G8R8A8_UNORM;
    case RHI_FORMAT_B8G8R8A8_SNORM:
        return VK_FORMAT_B8G8R8A8_SNORM;
    case RHI_FORMAT_B8G8R8A8_UINT:
        return VK_FORMAT_B8G8R8A8_UINT;
    case RHI_FORMAT_B8G8R8A8_SINT:
        return VK_FORMAT_B8G8R8A8_SINT;
    case RHI_FORMAT_B8G8R8A8_SRGB:
        return VK_FORMAT_B8G8R8A8_SRGB;
    case RHI_FORMAT_R32_UINT:
        return VK_FORMAT_R32_UINT;
    case RHI_FORMAT_R32_SINT:
        return VK_FORMAT_R32_SINT;
    case RHI_FORMAT_R32_FLOAT:
        return VK_FORMAT_R32_SFLOAT;
    case RHI_FORMAT_R32G32_UINT:
        return VK_FORMAT_R32G32_UINT;
    case RHI_FORMAT_R32G32_SINT:
        return VK_FORMAT_R32G32_SINT;
    case RHI_FORMAT_R32G32_FLOAT:
        return VK_FORMAT_R32G32_SFLOAT;
    case RHI_FORMAT_R32G32B32_UINT:
        return VK_FORMAT_R32G32B32_UINT;
    case RHI_FORMAT_R32G32B32_SINT:
        return VK_FORMAT_R32G32B32_SINT;
    case RHI_FORMAT_R32G32B32_FLOAT:
        return VK_FORMAT_R32G32B32_SFLOAT;
    case RHI_FORMAT_R32G32B32A32_UINT:
        return VK_FORMAT_R32G32B32A32_UINT;
    case RHI_FORMAT_R32G32B32A32_SINT:
        return VK_FORMAT_R32G32B32A32_SINT;
    case RHI_FORMAT_R32G32B32A32_FLOAT:
        return VK_FORMAT_R32G32B32A32_SFLOAT;
    case RHI_FORMAT_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case RHI_FORMAT_D32_FLOAT:
        return VK_FORMAT_D32_SFLOAT;
    default:
        return VK_FORMAT_UNDEFINED;
    }
}

rhi_format vk_util::map_format(VkFormat format)
{
    switch (format)
    {
    case VK_FORMAT_R8_UNORM:
        return RHI_FORMAT_R8_UNORM;
    case VK_FORMAT_R8_SNORM:
        return RHI_FORMAT_R8_SNORM;
    case VK_FORMAT_R8_UINT:
        return RHI_FORMAT_R8_UINT;
    case VK_FORMAT_R8_SINT:
        return RHI_FORMAT_R8_SINT;
    case VK_FORMAT_R8G8_UNORM:
        return RHI_FORMAT_R8G8_UNORM;
    case VK_FORMAT_R8G8_SNORM:
        return RHI_FORMAT_R8G8_SNORM;
    case VK_FORMAT_R8G8_UINT:
        return RHI_FORMAT_R8G8_UINT;
    case VK_FORMAT_R8G8_SINT:
        return RHI_FORMAT_R8G8_SINT;
    case VK_FORMAT_R8G8B8_UNORM:
        return RHI_FORMAT_R8G8B8_UNORM;
    case VK_FORMAT_R8G8B8_SNORM:
        return RHI_FORMAT_R8G8B8_SNORM;
    case VK_FORMAT_R8G8B8_UINT:
        return RHI_FORMAT_R8G8B8_UINT;
    case VK_FORMAT_R8G8B8_SINT:
        return RHI_FORMAT_R8G8B8_SINT;
    case VK_FORMAT_R8G8B8A8_UNORM:
        return RHI_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SNORM:
        return RHI_FORMAT_R8G8B8A8_SNORM;
    case VK_FORMAT_R8G8B8A8_UINT:
        return RHI_FORMAT_R8G8B8A8_UINT;
    case VK_FORMAT_R8G8B8A8_SINT:
        return RHI_FORMAT_R8G8B8A8_SINT;
    case VK_FORMAT_B8G8R8A8_UNORM:
        return RHI_FORMAT_B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SNORM:
        return RHI_FORMAT_B8G8R8A8_SNORM;
    case VK_FORMAT_B8G8R8A8_UINT:
        return RHI_FORMAT_B8G8R8A8_UINT;
    case VK_FORMAT_B8G8R8A8_SINT:
        return RHI_FORMAT_B8G8R8A8_SINT;
    case VK_FORMAT_B8G8R8A8_SRGB:
        return RHI_FORMAT_B8G8R8A8_SRGB;
    case VK_FORMAT_R32_UINT:
        return RHI_FORMAT_R32_UINT;
    case VK_FORMAT_R32_SINT:
        return RHI_FORMAT_R32_SINT;
    case VK_FORMAT_R32_SFLOAT:
        return RHI_FORMAT_R32_FLOAT;
    case VK_FORMAT_R32G32_UINT:
        return RHI_FORMAT_R32G32_UINT;
    case VK_FORMAT_R32G32_SINT:
        return RHI_FORMAT_R32G32_SINT;
    case VK_FORMAT_R32G32_SFLOAT:
        return RHI_FORMAT_R32G32_FLOAT;
    case VK_FORMAT_R32G32B32_UINT:
        return RHI_FORMAT_R32G32B32_UINT;
    case VK_FORMAT_R32G32B32_SINT:
        return RHI_FORMAT_R32G32B32_SINT;
    case VK_FORMAT_R32G32B32_SFLOAT:
        return RHI_FORMAT_R32G32B32_FLOAT;
    case VK_FORMAT_R32G32B32A32_UINT:
        return RHI_FORMAT_R32G32B32A32_UINT;
    case VK_FORMAT_R32G32B32A32_SINT:
        return RHI_FORMAT_R32G32B32A32_SINT;
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return RHI_FORMAT_R32G32B32A32_FLOAT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return RHI_FORMAT_D24_UNORM_S8_UINT;
    case VK_FORMAT_D32_SFLOAT:
        return RHI_FORMAT_D32_FLOAT;
    default:
        return RHI_FORMAT_UNDEFINED;
    }
}

VkSampleCountFlagBits vk_util::map_sample_count(rhi_sample_count samples)
{
    switch (samples)
    {
    case RHI_SAMPLE_COUNT_1:
        return VK_SAMPLE_COUNT_1_BIT;
    case RHI_SAMPLE_COUNT_2:
        return VK_SAMPLE_COUNT_2_BIT;
    case RHI_SAMPLE_COUNT_4:
        return VK_SAMPLE_COUNT_4_BIT;
    case RHI_SAMPLE_COUNT_8:
        return VK_SAMPLE_COUNT_8_BIT;
    case RHI_SAMPLE_COUNT_16:
        return VK_SAMPLE_COUNT_16_BIT;
    case RHI_SAMPLE_COUNT_32:
        return VK_SAMPLE_COUNT_32_BIT;
    default:
        return VK_SAMPLE_COUNT_1_BIT;
    }
}

VkImageLayout vk_util::map_layout(rhi_texture_layout layout)
{
    switch (layout)
    {
    case RHI_TEXTURE_LAYOUT_SHADER_RESOURCE:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case RHI_TEXTURE_LAYOUT_RENDER_TARGET:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RHI_TEXTURE_LAYOUT_DEPTH_STENCIL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case RHI_TEXTURE_LAYOUT_PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    default:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

VkFilter vk_util::map_filter(rhi_filter filter)
{
    switch (filter)
    {
    case RHI_FILTER_NEAREST:
        return VK_FILTER_NEAREST;
    case RHI_FILTER_LINEAR:
        return VK_FILTER_LINEAR;
    default:
        throw vk_exception("Invalid filter type.");
    }
}

VkSamplerAddressMode vk_util::map_sampler_address_mode(rhi_sampler_address_mode address_mode)
{
    switch (address_mode)
    {
    case RHI_SAMPLER_ADDRESS_MODE_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT:
        return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER:
        return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case RHI_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE:
        return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        throw vk_exception("Invalid sampler address mode.");
    }
}

VkPipelineStageFlags vk_util::map_pipeline_stage_flags(rhi_pipeline_stage_flags flags)
{
    VkPipelineStageFlags result = 0;
    result |= (flags & RHI_PIPELINE_STAGE_FLAG_BEGIN) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_FLAG_VERTEX) ? VK_PIPELINE_STAGE_VERTEX_SHADER_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_FLAG_EARLY_DEPTH_STENCIL)
                  ? VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                  : 0;
    result |=
        (flags & RHI_PIPELINE_STAGE_FLAG_FRAGMENT) ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_FLAG_LATE_DEPTH_STENCIL)
                  ? VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT
                  : 0;
    result |= (flags & RHI_PIPELINE_STAGE_FLAG_COLOR_OUTPUT)
                  ? VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
                  : 0;
    result |= (flags & RHI_PIPELINE_STAGE_FLAG_END) ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : 0;

    return result;
}

VkAccessFlags vk_util::map_access_flags(rhi_access_flags flags)
{
    VkAccessFlags result = 0;
    result |= (flags & RHI_ACCESS_FLAG_COLOR_READ) ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_FLAG_COLOR_WRITE) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0;
    result |= (flags & RHI_ACCESS_FLAG_DEPTH_STENCIL_READ)
                  ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                  : 0;
    result |= (flags & RHI_ACCESS_FLAG_DEPTH_STENCIL_WRITE)
                  ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT
                  : 0;
    result |= (flags & RHI_ACCESS_FLAG_SHADER_READ) ? VK_ACCESS_SHADER_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_FLAG_SHADER_WRITE) ? VK_ACCESS_SHADER_WRITE_BIT : 0;

    return result;
}
} // namespace violet::vk
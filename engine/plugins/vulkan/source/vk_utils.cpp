#include "vk_utils.hpp"

namespace violet::vk
{
VkFormat vk_utils::map_format(rhi_format format)
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
    case RHI_FORMAT_R8G8B8A8_SRGB:
        return VK_FORMAT_R8G8B8A8_SRGB;
    case RHI_FORMAT_B8G8R8_UNORM:
        return VK_FORMAT_B8G8R8_UNORM;
    case RHI_FORMAT_B8G8R8_SNORM:
        return VK_FORMAT_B8G8R8_SNORM;
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
    case RHI_FORMAT_R16G16_UNORM:
        return VK_FORMAT_R16G16_UNORM;
    case RHI_FORMAT_R16G16_FLOAT:
        return VK_FORMAT_R16G16_SFLOAT;
    case RHI_FORMAT_R16G16B16A16_UNORM:
        return VK_FORMAT_R16G16B16A16_UNORM;
    case RHI_FORMAT_R16G16B16A16_FLOAT:
        return VK_FORMAT_R16G16B16A16_SFLOAT;
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
    case RHI_FORMAT_R11G11B10_FLOAT:
        return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
    case RHI_FORMAT_D24_UNORM_S8_UINT:
        return VK_FORMAT_D24_UNORM_S8_UINT;
    case RHI_FORMAT_D32_FLOAT:
        return VK_FORMAT_D32_SFLOAT;
    case RHI_FORMAT_D32_FLOAT_S8_UINT:
        return VK_FORMAT_D32_SFLOAT_S8_UINT;
    default:
        throw std::runtime_error("Invalid format.");
    }
}

rhi_format vk_utils::map_format(VkFormat format)
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
    case VK_FORMAT_R8G8B8A8_SRGB:
        return RHI_FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_R8G8B8A8_SINT:
        return RHI_FORMAT_R8G8B8A8_SINT;
    case VK_FORMAT_B8G8R8_UNORM:
        return RHI_FORMAT_B8G8R8_UNORM;
    case VK_FORMAT_B8G8R8_SNORM:
        return RHI_FORMAT_B8G8R8_SNORM;
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
    case VK_FORMAT_R16G16_UNORM:
        return RHI_FORMAT_R16G16_UNORM;
    case VK_FORMAT_R16G16_SFLOAT:
        return RHI_FORMAT_R16G16_FLOAT;
    case VK_FORMAT_R16G16B16A16_UNORM:
        return RHI_FORMAT_R16G16B16A16_UNORM;
    case VK_FORMAT_R16G16B16A16_SFLOAT:
        return RHI_FORMAT_R16G16B16A16_FLOAT;
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
    case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
        return RHI_FORMAT_R11G11B10_FLOAT;
    case VK_FORMAT_D24_UNORM_S8_UINT:
        return RHI_FORMAT_D24_UNORM_S8_UINT;
    case VK_FORMAT_D32_SFLOAT:
        return RHI_FORMAT_D32_FLOAT;
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        return RHI_FORMAT_D32_FLOAT_S8_UINT;
    default:
        throw std::runtime_error("Invalid format.");
    }
}

VkSampleCountFlagBits vk_utils::map_sample_count(rhi_sample_count samples)
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
        throw std::runtime_error("Invalid sample count.");
    }
}

VkImageLayout vk_utils::map_layout(rhi_texture_layout layout)
{
    switch (layout)
    {
    case RHI_TEXTURE_LAYOUT_UNDEFINED:
        return VK_IMAGE_LAYOUT_UNDEFINED;
    case RHI_TEXTURE_LAYOUT_GENERAL:
        return VK_IMAGE_LAYOUT_GENERAL;
    case RHI_TEXTURE_LAYOUT_SHADER_RESOURCE:
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    case RHI_TEXTURE_LAYOUT_RENDER_TARGET:
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    case RHI_TEXTURE_LAYOUT_DEPTH_STENCIL:
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    case RHI_TEXTURE_LAYOUT_PRESENT:
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    case RHI_TEXTURE_LAYOUT_TRANSFER_SRC:
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    case RHI_TEXTURE_LAYOUT_TRANSFER_DST:
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    default:
        throw std::runtime_error("Invalid layout.");
    }
}

VkFilter vk_utils::map_filter(rhi_filter filter)
{
    switch (filter)
    {
    case RHI_FILTER_POINT:
        return VK_FILTER_NEAREST;
    case RHI_FILTER_LINEAR:
        return VK_FILTER_LINEAR;
    default:
        throw std::runtime_error("Invalid filter type.");
    }
}

VkSamplerAddressMode vk_utils::map_sampler_address_mode(rhi_sampler_address_mode address_mode)
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
        throw std::runtime_error("Invalid sampler address mode.");
    }
}

VkImageUsageFlags vk_utils::map_image_usage_flags(rhi_texture_flags flags)
{
    VkImageUsageFlags usages = 0;

    usages |= (flags & RHI_TEXTURE_SHADER_RESOURCE) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
    usages |= (flags & RHI_TEXTURE_RENDER_TARGET) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
    usages |= (flags & RHI_TEXTURE_DEPTH_STENCIL) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0;
    usages |= (flags & RHI_TEXTURE_TRANSFER_SRC) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
    usages |= (flags & RHI_TEXTURE_TRANSFER_DST) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
    usages |= (flags & RHI_TEXTURE_STORAGE) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;

    return usages;
}

VkBufferUsageFlags vk_utils::map_buffer_usage_flags(rhi_buffer_flags flags)
{
    VkBufferUsageFlags usages = 0;

    usages |= (flags & RHI_BUFFER_VERTEX) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
    usages |= (flags & RHI_BUFFER_INDEX) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
    usages |= (flags & RHI_BUFFER_UNIFORM) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
    usages |= (flags & RHI_BUFFER_UNIFORM_TEXEL) ? VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT : 0;
    usages |= (flags & RHI_BUFFER_STORAGE) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
    usages |= (flags & RHI_BUFFER_STORAGE_TEXEL) ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : 0;
    usages |= (flags & RHI_BUFFER_TRANSFER_SRC) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;
    usages |= (flags & RHI_BUFFER_TRANSFER_DST) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
    usages |= (flags & RHI_BUFFER_INDIRECT) ? VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT : 0;

    return usages;
}

VkPipelineStageFlags vk_utils::map_pipeline_stage_flags(rhi_pipeline_stage_flags flags)
{
    VkPipelineStageFlags result = 0;
    result |= (flags & RHI_PIPELINE_STAGE_BEGIN) ? VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_VERTEX_INPUT) ? VK_PIPELINE_STAGE_VERTEX_INPUT_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_VERTEX) ? VK_PIPELINE_STAGE_VERTEX_SHADER_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_EARLY_DEPTH_STENCIL) ?
                  VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT :
                  0;
    result |= (flags & RHI_PIPELINE_STAGE_FRAGMENT) ? VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_LATE_DEPTH_STENCIL) ?
                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT :
                  0;
    result |= (flags & RHI_PIPELINE_STAGE_COLOR_OUTPUT) ?
                  VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT :
                  0;
    result |= (flags & RHI_PIPELINE_STAGE_TRANSFER) ? VK_PIPELINE_STAGE_TRANSFER_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_COMPUTE) ? VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_END) ? VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_HOST) ? VK_PIPELINE_STAGE_HOST_BIT : 0;
    result |= (flags & RHI_PIPELINE_STAGE_DRAW_INDIRECT) ? VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT : 0;

    return result;
}

VkAccessFlags vk_utils::map_access_flags(rhi_access_flags flags)
{
    VkAccessFlags result = 0;
    result |= (flags & RHI_ACCESS_COLOR_READ) ? VK_ACCESS_COLOR_ATTACHMENT_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_COLOR_WRITE) ? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT : 0;
    result |=
        (flags & RHI_ACCESS_DEPTH_STENCIL_READ) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT : 0;
    result |=
        (flags & RHI_ACCESS_DEPTH_STENCIL_WRITE) ? VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT : 0;
    result |= (flags & RHI_ACCESS_SHADER_READ) ? VK_ACCESS_SHADER_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_SHADER_WRITE) ? VK_ACCESS_SHADER_WRITE_BIT : 0;
    result |= (flags & RHI_ACCESS_TRANSFER_READ) ? VK_ACCESS_TRANSFER_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_TRANSFER_WRITE) ? VK_ACCESS_TRANSFER_WRITE_BIT : 0;
    result |= (flags & RHI_ACCESS_HOST_READ) ? VK_ACCESS_HOST_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_HOST_WRITE) ? VK_ACCESS_HOST_WRITE_BIT : 0;
    result |= (flags & RHI_ACCESS_INDIRECT_COMMAND_READ) ? VK_ACCESS_INDIRECT_COMMAND_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_VERTEX_ATTRIBUTE_READ) ? VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT : 0;
    result |= (flags & RHI_ACCESS_INDEX_READ) ? VK_ACCESS_INDEX_READ_BIT : 0;

    return result;
}

VkBlendFactor vk_utils::map_blend_factor(rhi_blend_factor factor)
{
    switch (factor)
    {
    case RHI_BLEND_FACTOR_ZERO:
        return VK_BLEND_FACTOR_ZERO;
    case RHI_BLEND_FACTOR_ONE:
        return VK_BLEND_FACTOR_ONE;
    case RHI_BLEND_FACTOR_SRC_COLOR:
        return VK_BLEND_FACTOR_SRC_COLOR;
    case RHI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
    case RHI_BLEND_FACTOR_SRC_ALPHA:
        return VK_BLEND_FACTOR_SRC_ALPHA;
    case RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    case RHI_BLEND_FACTOR_DST_COLOR:
        return VK_BLEND_FACTOR_DST_COLOR;
    case RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
    case RHI_BLEND_FACTOR_DST_ALPHA:
        return VK_BLEND_FACTOR_DST_ALPHA;
    case RHI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA:
        return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    default:
        throw std::runtime_error("Invalid blend factor.");
    }
}

VkBlendOp vk_utils::map_blend_op(rhi_blend_op op)
{
    switch (op)
    {
    case RHI_BLEND_OP_ADD:
        return VK_BLEND_OP_ADD;
    case RHI_BLEND_OP_SUBTRACT:
        return VK_BLEND_OP_SUBTRACT;
    case RHI_BLEND_OP_MIN:
        return VK_BLEND_OP_MIN;
    case RHI_BLEND_OP_MAX:
        return VK_BLEND_OP_MAX;
    case RHI_BLEND_OP_MULTIPLY:
        return VK_BLEND_OP_MULTIPLY_EXT;
    default:
        throw std::runtime_error("Invalid blend op.");
    }
}

VkCompareOp vk_utils::map_compare_op(rhi_compare_op op)
{
    switch (op)
    {
    case RHI_COMPARE_OP_NEVER:
        return VK_COMPARE_OP_NEVER;
    case RHI_COMPARE_OP_LESS:
        return VK_COMPARE_OP_LESS;
    case RHI_COMPARE_OP_EQUAL:
        return VK_COMPARE_OP_EQUAL;
    case RHI_COMPARE_OP_LESS_EQUAL:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case RHI_COMPARE_OP_GREATER:
        return VK_COMPARE_OP_GREATER;
    case RHI_COMPARE_OP_NOT_EQUAL:
        return VK_COMPARE_OP_NOT_EQUAL;
    case RHI_COMPARE_OP_GREATER_EQUAL:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case RHI_COMPARE_OP_ALWAYS:
        return VK_COMPARE_OP_ALWAYS;
    default:
        throw std::runtime_error("Invalid compare op.");
    }
}

VkStencilOp vk_utils::map_stencil_op(rhi_stencil_op op)
{
    switch (op)
    {
    case RHI_STENCIL_OP_KEEP:
        return VK_STENCIL_OP_KEEP;
    case RHI_STENCIL_OP_ZERO:
        return VK_STENCIL_OP_ZERO;
    case RHI_STENCIL_OP_REPLACE:
        return VK_STENCIL_OP_REPLACE;
    case RHI_STENCIL_OP_INCR:
        return VK_STENCIL_OP_INCREMENT_AND_WRAP;
    case RHI_STENCIL_OP_DECR:
        return VK_STENCIL_OP_DECREMENT_AND_WRAP;
    case RHI_STENCIL_OP_INCR_CLAMP:
        return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
    case RHI_STENCIL_OP_DECR_CLAMP:
        return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
    case RHI_STENCIL_OP_INVERT:
        return VK_STENCIL_OP_INVERT;
    default:
        throw std::runtime_error("Invalid stencil op.");
    }
}
} // namespace violet::vk
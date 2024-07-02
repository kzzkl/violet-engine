#pragma once

#include "common/hash.hpp"
#include "vk_common.hpp"

namespace violet::vk
{
class vk_util
{
public:
    static VkFormat map_format(rhi_format format);
    static rhi_format map_format(VkFormat format);

    static VkSampleCountFlagBits map_sample_count(rhi_sample_count samples);

    static VkImageLayout map_layout(rhi_texture_layout layout);

    static VkFilter map_filter(rhi_filter filter);
    static VkSamplerAddressMode map_sampler_address_mode(rhi_sampler_address_mode address_mode);

    static VkPipelineStageFlags map_pipeline_stage_flags(rhi_pipeline_stage_flags flags);
    static VkAccessFlags map_access_flags(rhi_access_flags flags);

    static VkBlendFactor map_blend_factor(rhi_blend_factor factor);
    static VkBlendOp map_blend_op(rhi_blend_op op);
};
} // namespace violet::vk
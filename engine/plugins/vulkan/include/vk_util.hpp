#pragma once

#include "vk_common.hpp"

namespace violet::vk
{
class vk_util
{
public:
    static VkFormat map_format(rhi_resource_format format);
    static rhi_resource_format map_format(VkFormat format);

    static VkSampleCountFlagBits map_sample_count(rhi_sample_count samples);

    static VkImageLayout map_state(rhi_resource_state state);

    static VkFilter map_filter(rhi_filter filter);
    static VkSamplerAddressMode map_sampler_address_mode(rhi_sampler_address_mode address_mode);

    static inline std::size_t hash_combine(std::size_t seed, std::size_t value)
    {
        return seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }
};
} // namespace violet::vk
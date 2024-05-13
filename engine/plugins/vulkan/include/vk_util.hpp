#pragma once

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

    static inline void hash_combine(std::size_t& seed, std::size_t value)
    {
        seed = seed ^ (value + 0x9e3779b9 + (seed << 6) + (seed >> 2));
    }

    template <typename... Args>
    static std::size_t hash(Args&&... args)
    {
        std::size_t result = 0;
        (hash_combine(result, hash_impl(args)), ...);
        return result;
    }

private:
    template <typename T>
    static inline std::size_t hash_impl(const T& value)
    {
        std::hash<T> hasher;
        return hasher(value);
    }
};
} // namespace violet::vk
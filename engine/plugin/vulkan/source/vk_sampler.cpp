#include "vk_sampler.hpp"
#include "vk_context.hpp"

namespace ash::graphics::vk
{
vk_sampler::vk_sampler()
{
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    auto device = vk_context::device();
    throw_if_failed(vkCreateSampler(device, &sampler_info, nullptr, &m_sampler));
}

vk_sampler::vk_sampler(vk_sampler&& other) : m_sampler(other.m_sampler)
{
    other.m_sampler = VK_NULL_HANDLE;
}

vk_sampler::~vk_sampler()
{
    if (m_sampler != VK_NULL_HANDLE)
    {
        auto device = vk_context::device();
        vkDestroySampler(device, m_sampler, nullptr);
    }
}

vk_sampler& vk_sampler::operator=(vk_sampler&& other)
{
    m_sampler = other.m_sampler;
    other.m_sampler = VK_NULL_HANDLE;

    return *this;
}
} // namespace ash::graphics::vk
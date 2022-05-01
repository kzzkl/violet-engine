#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_sampler
{
public:
    vk_sampler();
    vk_sampler(vk_sampler&& other);
    virtual ~vk_sampler();

    vk_sampler& operator=(vk_sampler&& other);

    VkSampler sampler() const noexcept { return m_sampler; }

private:
    VkSampler m_sampler;
};
} // namespace ash::graphics::vk
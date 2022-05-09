#pragma once

#include "vk_common.hpp"

namespace ash::graphics::vk
{
class vk_descriptor_pool
{
public:
    vk_descriptor_pool();
    ~vk_descriptor_pool();

    VkDescriptorSet allocate_descriptor_set(VkDescriptorSetLayout layout);

private:
    VkDescriptorPool m_descriptor_pool;
};
} // namespace ash::graphics::vk
#include "vk_descriptor_pool.hpp"
#include "vk_context.hpp"

namespace ash::graphics::vk
{
vk_descriptor_pool::vk_descriptor_pool()
{
    VkDescriptorPoolSize uniform_buffer_size = {};
    uniform_buffer_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uniform_buffer_size.descriptorCount = 1024;

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &uniform_buffer_size;
    pool_info.maxSets = 512;

    auto device = vk_context::device();
    throw_if_failed(vkCreateDescriptorPool(device, &pool_info, nullptr, &m_descriptor_pool));
}

vk_descriptor_pool::~vk_descriptor_pool()
{
    auto device = vk_context::device();
    vkDestroyDescriptorPool(device, m_descriptor_pool, nullptr);
}

VkDescriptorSet vk_descriptor_pool::allocate_descriptor_set(VkDescriptorSetLayout layout)
{
    VkDescriptorSetAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocate_info.descriptorPool = m_descriptor_pool;
    allocate_info.descriptorSetCount = 1;
    allocate_info.pSetLayouts = &layout;

    VkDescriptorSet result;

    auto device = vk_context::device();
    throw_if_failed(vkAllocateDescriptorSets(device, &allocate_info, &result));

    return result;
}
} // namespace ash::graphics::vk
#include "vk_layout.hpp"
#include "vk_context.hpp"
#include <cassert>

namespace violet::vk
{
vk_parameter_layout::vk_parameter_layout(
    std::span<const rhi_parameter_binding> bindings,
    vk_context* context)
    : m_layout(VK_NULL_HANDLE),
      m_bindings(bindings.size()),
      m_context(context)
{
    std::vector<VkDescriptorSetLayoutBinding> layout_bindings;
    std::vector<VkDescriptorBindingFlags> binding_flags;

    std::size_t uniform_count = 0;
    std::size_t mutable_count = 0;
    bool bindless = false;

    for (std::size_t i = 0; i < m_bindings.size(); ++i)
    {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = static_cast<std::uint32_t>(i);
        binding.stageFlags |=
            bindings[i].stages & RHI_SHADER_STAGE_VERTEX ? VK_SHADER_STAGE_VERTEX_BIT : 0;
        binding.stageFlags |=
            bindings[i].stages & RHI_SHADER_STAGE_GEOMETRY ? VK_SHADER_STAGE_GEOMETRY_BIT : 0;
        binding.stageFlags |=
            bindings[i].stages & RHI_SHADER_STAGE_FRAGMENT ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        binding.stageFlags |=
            bindings[i].stages & RHI_SHADER_STAGE_COMPUTE ? VK_SHADER_STAGE_COMPUTE_BIT : 0;

        assert(binding.stageFlags != 0);

        binding.pImmutableSamplers = nullptr;

        m_bindings[i].type = bindings[i].type;
        m_bindings[i].size = bindings[i].size;

        // When size is 0, it indicates bindless descriptor. Otherwise, it indicates the number of
        // descriptors.
        if (bindings[i].size == 0)
        {
            binding.descriptorCount =
                bindings[i].type == RHI_PARAMETER_BINDING_TYPE_SAMPLER ? 512 : 65536;
            binding_flags.push_back(
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);

            bindless = true;
        }
        else
        {
            if (bindings[i].type == RHI_PARAMETER_BINDING_TYPE_UNIFORM)
            {
                binding.descriptorCount = 1;
            }
            else
            {
                binding.descriptorCount = static_cast<std::uint32_t>(bindings[i].size);
            }
            binding_flags.push_back(0);
        }

        switch (bindings[i].type)
        {
        case RHI_PARAMETER_BINDING_TYPE_UNIFORM: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            m_bindings[i].uniform.index = uniform_count;

            ++uniform_count;
            break;
        }
        case RHI_PARAMETER_BINDING_TYPE_STORAGE_BUFFER: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        }
        case RHI_PARAMETER_BINDING_TYPE_STORAGE_TEXEL: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            break;
        }
        case RHI_PARAMETER_BINDING_TYPE_STORAGE_TEXTURE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            break;
        }
        case RHI_PARAMETER_BINDING_TYPE_TEXTURE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        }
        case RHI_PARAMETER_BINDING_TYPE_SAMPLER: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        }
        case RHI_PARAMETER_BINDING_TYPE_MUTABLE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
            ++mutable_count;
            break;
        }
        default:
            break;
        }

        layout_bindings.push_back(binding);
    }

    assert(mutable_count <= 1);

    VkDescriptorType mutable_descriptor_types[] = {
        VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
        VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    };

    VkMutableDescriptorTypeListEXT mutable_descriptor_list = {
        .descriptorTypeCount = 6,
        .pDescriptorTypes = mutable_descriptor_types,
    };

    VkMutableDescriptorTypeCreateInfoEXT mutable_descriptor_info = {
        .sType = VK_STRUCTURE_TYPE_MUTABLE_DESCRIPTOR_TYPE_CREATE_INFO_EXT,
        .mutableDescriptorTypeListCount = 1,
        .pMutableDescriptorTypeLists = &mutable_descriptor_list,
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo descriptor_set_layout_binding_flags = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .pNext = mutable_count > 0 ? &mutable_descriptor_info : nullptr,
        .bindingCount = static_cast<std::uint32_t>(binding_flags.size()),
        .pBindingFlags = binding_flags.data(),
    };

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descriptor_set_layout_binding_flags,
        .bindingCount = static_cast<std::uint32_t>(layout_bindings.size()),
        .pBindings = layout_bindings.data(),
    };

    if (bindless)
    {
        descriptor_set_layout_info.flags =
            VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
    }

    vk_check(vkCreateDescriptorSetLayout(
        m_context->get_device(),
        &descriptor_set_layout_info,
        nullptr,
        &m_layout));
}

vk_parameter_layout::~vk_parameter_layout()
{
    vkDestroyDescriptorSetLayout(m_context->get_device(), m_layout, nullptr);
}

vk_pipeline_layout::vk_pipeline_layout(
    std::uint32_t push_constant_size,
    VkPipelineStageFlags push_constant_stages,
    std::span<const vk_parameter_layout*> parameters,
    vk_context* context)
    : m_push_constant_size(push_constant_size),
      m_push_constant_stages(push_constant_stages),
      m_context(context)
{
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    for (const vk_parameter_layout* parameter : parameters)
    {
        if (parameter == nullptr)
        {
            break;
        }

        descriptor_set_layouts.push_back(parameter->get_layout());
    }

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    layout_info.setLayoutCount = static_cast<std::uint32_t>(descriptor_set_layouts.size());

    VkPushConstantRange push_constant_range = {
        .stageFlags = push_constant_stages,
        .size = push_constant_size,
    };
    if (push_constant_size != 0)
    {
        layout_info.pushConstantRangeCount = 1;
        layout_info.pPushConstantRanges = &push_constant_range;
    }

    vk_check(vkCreatePipelineLayout(m_context->get_device(), &layout_info, nullptr, &m_layout));
}

vk_pipeline_layout::~vk_pipeline_layout()
{
    vkDestroyPipelineLayout(m_context->get_device(), m_layout, nullptr);
}

vk_layout_manager::vk_layout_manager(vk_context* context)
    : m_context(context)
{
}

vk_layout_manager::~vk_layout_manager() {}

vk_parameter_layout* vk_layout_manager::get_parameter_layout(
    std::span<const rhi_parameter_binding> bindings)
{
    parameter_layout_key key = {};
    for (std::size_t i = 0; i < bindings.size(); ++i)
    {
        key.bindings[i].type = bindings[i].type;
        key.bindings[i].stages = bindings[i].stages;
        key.bindings[i].size = bindings[i].size;
    }
    key.binding_count = static_cast<std::uint32_t>(bindings.size());

    auto iter = m_parameter_layouts.find(key);
    if (iter != m_parameter_layouts.end())
    {
        return iter->second.get();
    }

    return (m_parameter_layouts[key] = std::make_unique<vk_parameter_layout>(bindings, m_context))
        .get();
}

vk_pipeline_layout* vk_layout_manager::get_pipeline_layout(
    std::uint32_t push_constant_size,
    VkPipelineStageFlags push_constant_stages,
    std::span<const vk_parameter_layout*> parameters)
{
    pipeline_layout_key key = {};
    key.push_constant_size = push_constant_size;
    key.push_constant_stages = push_constant_stages;
    for (std::size_t i = 0; i < parameters.size(); ++i)
    {
        key.parameters[i] = parameters[i];
    }

    auto iter = m_pipeline_layouts.find(key);
    if (iter != m_pipeline_layouts.end())
    {
        return iter->second.get();
    }

    return (m_pipeline_layouts[key] = std::make_unique<vk_pipeline_layout>(
                push_constant_size,
                push_constant_stages,
                parameters,
                m_context))
        .get();
}
} // namespace violet::vk
#include "vk_layout.hpp"
#include "common/hash.hpp"
#include "vk_context.hpp"
#include <algorithm>
#include <cassert>

namespace violet::vk
{
vk_parameter_layout::vk_parameter_layout(const rhi_parameter_desc& desc, vk_context* context)
    : m_layout(VK_NULL_HANDLE),
      m_context(context),
      m_bindings(desc.binding_count)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorBindingFlags> binding_flags;

    std::size_t constant_count = 0;
    std::size_t mutable_count = 0;
    bool bindless = false;

    for (std::size_t i = 0; i < m_bindings.size(); ++i)
    {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = static_cast<std::uint32_t>(i);
        binding.stageFlags |=
            desc.bindings[i].stages & RHI_SHADER_STAGE_VERTEX ? VK_SHADER_STAGE_VERTEX_BIT : 0;
        binding.stageFlags |=
            desc.bindings[i].stages & RHI_SHADER_STAGE_FRAGMENT ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
        binding.stageFlags |=
            desc.bindings[i].stages & RHI_SHADER_STAGE_COMPUTE ? VK_SHADER_STAGE_COMPUTE_BIT : 0;

        binding.pImmutableSamplers = nullptr;

        m_bindings[i].type = desc.bindings[i].type;
        m_bindings[i].size = desc.bindings[i].size;

        // When size is 0, it indicates bindless descriptor. Otherwise, it indicates the number of descriptors.
        if (desc.bindings[i].size == 0)
        {
            binding.descriptorCount = 65536;
            binding_flags.push_back(
                VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
                VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT);

            bindless = true;
        }
        else
        {
            if (desc.bindings[i].type == RHI_PARAMETER_BINDING_CONSTANT)
            {
                binding.descriptorCount = 1;
            }
            else
            {
                binding.descriptorCount = desc.bindings[i].size;
            }
            binding_flags.push_back(0);
        }

        switch (desc.bindings[i].type)
        {
        case RHI_PARAMETER_BINDING_CONSTANT: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            m_bindings[i].constant.index = constant_count;

            ++constant_count;
            break;
        }
        case RHI_PARAMETER_BINDING_UNIFORM: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        }
        case RHI_PARAMETER_BINDING_UNIFORM_TEXEL: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
            break;
        }
        case RHI_PARAMETER_BINDING_STORAGE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            break;
        }
        case RHI_PARAMETER_BINDING_STORAGE_TEXEL: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            break;
        }
        case RHI_PARAMETER_BINDING_TEXTURE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            break;
        }
        case RHI_PARAMETER_BINDING_SAMPLER: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
            break;
        }
        case RHI_PARAMETER_BINDING_MUTABLE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_MUTABLE_EXT;
            ++mutable_count;
            break;
        }
        default:
            break;
        }

        bindings.push_back(binding);
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
        .bindingCount = static_cast<std::uint32_t>(bindings.size()),
        .pBindings = bindings.data(),
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
    std::span<vk_parameter_layout*> parameter_layouts,
    vk_context* context)
    : m_layout(VK_NULL_HANDLE),
      m_context(context)
{
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts(parameter_layouts.size());
    std::transform(
        parameter_layouts.begin(),
        parameter_layouts.end(),
        descriptor_set_layouts.begin(),
        [](vk_parameter_layout* parameter_layout) -> VkDescriptorSetLayout
        {
            if (parameter_layout != nullptr)
            {
                return parameter_layout->get_layout();
            }

            return VK_NULL_HANDLE;
        });

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    layout_info.setLayoutCount = static_cast<std::uint32_t>(descriptor_set_layouts.size());
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

vk_parameter_layout* vk_layout_manager::get_parameter_layout(const rhi_parameter_desc& desc)
{
    std::uint64_t hash =
        hash::city_hash_64(desc.bindings, sizeof(rhi_parameter_binding) * desc.binding_count);
    auto iter = m_parameter_layouts.find(hash);
    if (iter != m_parameter_layouts.end())
    {
        return iter->second.get();
    }

    auto layout = std::make_unique<vk_parameter_layout>(desc, m_context);
    vk_parameter_layout* result = layout.get();
    m_parameter_layouts[hash] = std::move(layout);
    return result;
}

vk_pipeline_layout* vk_layout_manager::get_pipeline_layout(
    std::span<vk_parameter_layout*> parameter_layouts)
{
    std::uint64_t hash = 0;
    for (auto& parameter_layout : parameter_layouts)
    {
        hash = hash::combine(
            hash,
            hash::city_hash_64(&parameter_layout, sizeof(vk_parameter_layout*)));
    }

    auto iter = m_pipeline_layouts.find(hash);
    if (iter != m_pipeline_layouts.end())
    {
        return iter->second.get();
    }

    auto layout = std::make_unique<vk_pipeline_layout>(parameter_layouts, m_context);
    vk_pipeline_layout* result = layout.get();
    m_pipeline_layouts[hash] = std::move(layout);
    return result;
}
} // namespace violet::vk
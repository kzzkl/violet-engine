#pragma once

#include "vk_common.hpp"
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

namespace violet::vk
{
class vk_context;

class vk_parameter_layout
{
public:
    struct binding
    {
        rhi_parameter_binding_type type;
        std::size_t size;

        struct
        {
            std::size_t index;
        } constant;
    };

    vk_parameter_layout(const rhi_parameter_desc& desc, vk_context* context);
    ~vk_parameter_layout();

    const std::vector<binding>& get_bindings() const noexcept
    {
        return m_bindings;
    }

    VkDescriptorSetLayout get_layout() const noexcept
    {
        return m_layout;
    }

private:
    VkDescriptorSetLayout m_layout;
    std::vector<binding> m_bindings;

    vk_context* m_context;
};

class vk_pipeline_layout
{
public:
    vk_pipeline_layout(std::span<vk_parameter_layout*> parameter_layouts, vk_context* context);
    ~vk_pipeline_layout();

    VkPipelineLayout get_layout() const noexcept
    {
        return m_layout;
    }

private:
    VkPipelineLayout m_layout;
    vk_context* m_context;
};

class vk_layout_manager
{
public:
    vk_layout_manager(vk_context* context);
    ~vk_layout_manager();

    vk_parameter_layout* get_parameter_layout(const rhi_parameter_desc& desc);
    vk_pipeline_layout* get_pipeline_layout(std::span<vk_parameter_layout*> parameter_layouts);

private:
    std::unordered_map<std::uint64_t, std::unique_ptr<vk_parameter_layout>> m_parameter_layouts;
    std::unordered_map<std::uint64_t, std::unique_ptr<vk_pipeline_layout>> m_pipeline_layouts;

    vk_context* m_context;
};
} // namespace violet::vk
#pragma once

#include "algorithm/hash.hpp"
#include "vk_common.hpp"
#include <array>
#include <memory>
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
        } uniform;
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

struct vk_pipeline_layout_desc
{
    bool operator==(const vk_pipeline_layout_desc& other) const noexcept
    {
        if (push_constant_stages != other.push_constant_stages ||
            push_constant_size != other.push_constant_size)
        {
            return false;
        }

        for (std::size_t i = 0; i < parameters.size(); ++i)
        {
            if (parameters[i] != other.parameters[i])
            {
                return false;
            }
        }

        return true;
    }

    VkPipelineStageFlags push_constant_stages;
    std::uint32_t push_constant_size;

    std::array<vk_parameter_layout*, rhi_constants::max_parameters> parameters;
};

class vk_pipeline_layout
{
public:
    vk_pipeline_layout(const vk_pipeline_layout_desc& desc, vk_context* context);
    ~vk_pipeline_layout();

    VkPipelineLayout get_layout() const noexcept
    {
        return m_layout;
    }

    VkShaderStageFlags get_push_constant_stages() const noexcept
    {
        return m_push_constant_stages;
    }

    std::size_t get_push_constant_size() const noexcept
    {
        return m_push_constant_size;
    }

private:
    VkPipelineLayout m_layout;
    VkShaderStageFlags m_push_constant_stages;
    std::size_t m_push_constant_size{0};

    vk_context* m_context;
};

class vk_layout_manager
{
public:
    vk_layout_manager(vk_context* context);
    ~vk_layout_manager();

    vk_parameter_layout* get_parameter_layout(const rhi_parameter_desc& desc);
    vk_pipeline_layout* get_pipeline_layout(const vk_pipeline_layout_desc& desc);

private:
    struct pipeline_layout_hash
    {
        std::size_t operator()(const violet::vk::vk_pipeline_layout_desc& desc) const noexcept
        {
            return violet::hash::city_hash_64(desc);
        }
    };

    std::unordered_map<std::uint64_t, std::unique_ptr<vk_parameter_layout>> m_parameter_layouts;
    std::unordered_map<
        vk_pipeline_layout_desc,
        std::unique_ptr<vk_pipeline_layout>,
        pipeline_layout_hash>
        m_pipeline_layouts;

    vk_context* m_context;
};
} // namespace violet::vk
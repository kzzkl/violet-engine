#pragma once

#include "algorithm/hash.hpp"
#include "vk_common.hpp"
#include <array>
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
        } uniform;
    };

    vk_parameter_layout(std::span<const rhi_parameter_binding> bindings, vk_context* context);
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
    vk_pipeline_layout(
        std::uint32_t push_constant_size,
        VkPipelineStageFlags push_constant_stages,
        std::span<const vk_parameter_layout*> parameters,
        vk_context* context);
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
    VkPipelineLayout m_layout{VK_NULL_HANDLE};
    std::size_t m_push_constant_size{0};
    VkShaderStageFlags m_push_constant_stages;

    vk_context* m_context;
};

class vk_layout_manager
{
public:
    vk_layout_manager(vk_context* context);
    ~vk_layout_manager();

    vk_parameter_layout* get_parameter_layout(std::span<const rhi_parameter_binding> bindings);
    vk_pipeline_layout* get_pipeline_layout(
        std::uint32_t push_constant_size,
        VkPipelineStageFlags push_constant_stages,
        std::span<const vk_parameter_layout*> parameters);

private:
    struct parameter_layout_key
    {
        rhi_parameter_binding bindings[rhi_constants::max_parameter_bindings];
        std::uint32_t binding_count;

        bool operator==(const parameter_layout_key& other) const noexcept
        {
            if (binding_count != other.binding_count)
            {
                return false;
            }

            for (std::size_t i = 0; i < binding_count; ++i)
            {
                if (bindings[i].type != other.bindings[i].type ||
                    bindings[i].stages != other.bindings[i].stages ||
                    bindings[i].size != other.bindings[i].size)
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct parameter_layout_hash
    {
        std::uint64_t operator()(const parameter_layout_key& key) const noexcept
        {
            return hash::xx_hash(&key, sizeof(parameter_layout_key));
        }
    };

    struct pipeline_layout_key
    {
        VkPipelineStageFlags push_constant_stages;
        std::uint32_t push_constant_size;
        std::array<const vk_parameter_layout*, rhi_constants::max_parameters> parameters;

        bool operator==(const pipeline_layout_key& other) const noexcept
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
    };

    struct pipeline_layout_hash
    {
        std::uint64_t operator()(const pipeline_layout_key& desc) const noexcept
        {
            return hash::xx_hash(&desc, sizeof(desc));
        }
    };

    std::unordered_map<
        parameter_layout_key,
        std::unique_ptr<vk_parameter_layout>,
        parameter_layout_hash>
        m_parameter_layouts;
    std::unordered_map<
        pipeline_layout_key,
        std::unique_ptr<vk_pipeline_layout>,
        pipeline_layout_hash>
        m_pipeline_layouts;

    vk_context* m_context;
};
} // namespace violet::vk
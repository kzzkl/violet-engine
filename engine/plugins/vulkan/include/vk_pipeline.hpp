#pragma once

#include "vk_common.hpp"
#include <memory>
#include <string>
#include <vector>

namespace violet::vk
{
class vk_rhi;
class vk_uniform_buffer;
class vk_pipeline_parameter_layout : public rhi_pipeline_parameter_layout
{
public:
    struct parameter_info
    {
        std::size_t index;
        VkDescriptorType type;

        union {
            struct
            {
                std::size_t size;
            } uniform_buffer;
        };
    };

public:
    vk_pipeline_parameter_layout(const rhi_pipeline_parameter_layout_desc& desc, vk_rhi* rhi);
    virtual ~vk_pipeline_parameter_layout();

    const std::vector<parameter_info>& get_parameter_infos() const noexcept
    {
        return m_parameter_infos;
    }

    VkDescriptorSetLayout get_layout() const noexcept { return m_layout; }

private:
    VkDescriptorSetLayout m_layout;
    std::vector<parameter_info> m_parameter_infos;

    vk_rhi* m_rhi;
};

class vk_pipeline_parameter : public rhi_pipeline_parameter
{
public:
    vk_pipeline_parameter(vk_pipeline_parameter_layout* layout, vk_rhi* rhi);
    virtual ~vk_pipeline_parameter();

    virtual void set(std::size_t index, const void* data, std::size_t size, std::size_t offset)
        override;
    virtual void set(std::size_t index, rhi_resource* texture) override;

    VkDescriptorSet get_descriptor_set() const noexcept;

    void sync();

private:
    vk_pipeline_parameter_layout* m_layout;
    std::vector<std::unique_ptr<vk_uniform_buffer>> m_uniform_buffers;
    std::vector<VkDescriptorSet> m_descriptor_sets;

    std::size_t m_last_update_frame;
    std::size_t m_last_sync_frame;
    std::vector<std::size_t> m_parameter_update_frame;

    vk_rhi* m_rhi;
};

class vk_render_pipeline : public rhi_render_pipeline
{
public:
    vk_render_pipeline(const rhi_render_pipeline_desc& desc, VkExtent2D extent, vk_rhi* rhi);
    vk_render_pipeline(const vk_render_pipeline&) = delete;
    virtual ~vk_render_pipeline();

    VkPipeline get_pipeline() const noexcept { return m_pipeline; }
    VkPipelineLayout get_pipeline_layout() const noexcept { return m_pipeline_layout; }

    vk_render_pipeline& operator=(const vk_render_pipeline&) = delete;

private:
    VkShaderModule load_shader(std::string_view path);

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;

    vk_rhi* m_rhi;
};
} // namespace violet::vk

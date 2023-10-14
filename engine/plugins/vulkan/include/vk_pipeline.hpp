#pragma once

#include "vk_common.hpp"
#include "vk_context.hpp"
#include <memory>
#include <string>

namespace violet::vk
{
class vk_uniform_buffer;
class vk_parameter_layout : public rhi_parameter_layout
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
    vk_parameter_layout(const rhi_parameter_layout_desc& desc, vk_context* context);
    virtual ~vk_parameter_layout();

    const std::vector<parameter_info>& get_parameter_infos() const noexcept
    {
        return m_parameter_infos;
    }

    VkDescriptorSetLayout get_layout() const noexcept { return m_layout; }

private:
    VkDescriptorSetLayout m_layout;
    std::vector<parameter_info> m_parameter_infos;

    vk_context* m_context;
};

class vk_parameter : public rhi_parameter
{
public:
    vk_parameter(vk_parameter_layout* layout, vk_context* context);
    virtual ~vk_parameter();

    virtual void set(std::size_t index, const void* data, std::size_t size, std::size_t offset)
        override;
    virtual void set(std::size_t index, rhi_resource* texture, rhi_sampler* sampler) override;

    VkDescriptorSet get_descriptor_set() const noexcept;

    void sync();

private:
    struct frame_resource
    {
        VkDescriptorSet descriptor_set;
        std::vector<std::size_t> descriptor_update_count;

        std::size_t update_count;
    };

    void mark_dirty(std::size_t descriptor_index);

    vk_parameter_layout* m_layout;
    std::vector<std::unique_ptr<vk_uniform_buffer>> m_uniform_buffers;
    std::vector<frame_resource> m_frame_resources;

    vk_context* m_context;
};

class vk_render_pipeline : public rhi_render_pipeline
{
public:
    vk_render_pipeline(
        const rhi_render_pipeline_desc& desc,
        VkExtent2D extent,
        vk_context* context);
    vk_render_pipeline(const vk_render_pipeline&) = delete;
    virtual ~vk_render_pipeline();

    VkPipeline get_pipeline() const noexcept { return m_pipeline; }
    VkPipelineLayout get_pipeline_layout() const noexcept { return m_pipeline_layout; }

    vk_render_pipeline& operator=(const vk_render_pipeline&) = delete;

private:
    VkShaderModule load_shader(std::string_view path);

    VkPipeline m_pipeline;
    VkPipelineLayout m_pipeline_layout;

    vk_context* m_context;
};
} // namespace violet::vk

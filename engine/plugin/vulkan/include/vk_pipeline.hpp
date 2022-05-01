#pragma once

#include "vk_common.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
class vk_pipeline_parameter_layout : public pipeline_parameter_layout
{
public:
    vk_pipeline_parameter_layout(const pipeline_parameter_layout_desc& desc);

    VkDescriptorSetLayout layout() const noexcept { return m_descriptor_set_layout; }
    const std::vector<pipeline_parameter_pair>& parameters() const noexcept { return m_parameters; }

private:
    VkDescriptorSetLayout m_descriptor_set_layout;
    std::vector<pipeline_parameter_pair> m_parameters;
};

class vk_pipeline_parameter : public pipeline_parameter
{
public:
    vk_pipeline_parameter(pipeline_parameter_layout* layout);

    virtual void set(std::size_t index, bool value) override {}
    virtual void set(std::size_t index, std::uint32_t value) override {}
    virtual void set(std::size_t index, float value) override {}
    virtual void set(std::size_t index, const math::float2& value) override {}
    virtual void set(std::size_t index, const math::float3& value) override;
    virtual void set(std::size_t index, const math::float4& value) override {}
    virtual void set(std::size_t index, const math::float4x4& value, bool row_matrix = true)
        override
    {
    }
    virtual void set(
        std::size_t index,
        const math::float4x4* data,
        size_t size,
        bool row_matrix = true) override
    {
    }
    virtual void set(std::size_t index, resource* texture) override {}

    VkDescriptorSet descriptor_set() const noexcept { return m_descriptor_set; }

private:
    VkDescriptorSet m_descriptor_set;

    std::unique_ptr<vk_uniform_buffer> m_buffer;
};

class vk_pipeline_layout : public pipeline_layout
{
public:
    vk_pipeline_layout(const pipeline_layout_desc& desc);
    virtual ~vk_pipeline_layout();

    VkPipelineLayout pipeline_layout() const noexcept { return m_pipeline_layout; }

private:
    VkPipelineLayout m_pipeline_layout;
};

class vk_pipeline
{
public:
    vk_pipeline(const pipeline_desc& desc, VkRenderPass render_pass, std::size_t index);

    void begin(VkCommandBuffer command_buffer);
    void end(VkCommandBuffer command_buffer);

    VkPipelineLayout layout() const noexcept { return m_pipeline_layout->pipeline_layout(); }
    VkPipeline pipeline() const noexcept { return m_pipeline; }

private:
    VkShaderModule load_shader(std::string_view file);

    vk_pipeline_layout* m_pipeline_layout;
    VkPipeline m_pipeline;
};

class vk_render_pass : public render_pass
{
public:
    vk_render_pass(const render_pass_desc& desc);
    ~vk_render_pass();

    void begin(VkCommandBuffer command_buffer, VkFramebuffer frame_buffer);
    void end(VkCommandBuffer command_buffer);
    void next(VkCommandBuffer command_buffer);

    VkRenderPass render_pass() const noexcept { return m_render_pass; }
    vk_pipeline& current_subpass() { return m_pipelines[m_subpass_index]; }

private:
    void create_pass(const render_pass_desc& desc);

    VkRenderPass m_render_pass;
    std::vector<vk_pipeline> m_pipelines;

    std::size_t m_subpass_index;
};
} // namespace ash::graphics::vk
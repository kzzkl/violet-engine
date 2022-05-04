#pragma once

#include "vk_common.hpp"
#include "vk_resource.hpp"

namespace ash::graphics::vk
{
class vk_pass_parameter_layout : public pass_parameter_layout_interface
{
public:
    vk_pass_parameter_layout(const pass_parameter_layout_desc& desc);

    VkDescriptorSetLayout layout() const noexcept { return m_descriptor_set_layout; }
    const std::vector<pass_parameter_pair>& parameters() const noexcept { return m_parameters; }

    std::pair<std::size_t, std::size_t> descriptor_count() const noexcept
    {
        return {m_ubo_count, m_cis_count};
    }

private:
    VkDescriptorSetLayout m_descriptor_set_layout;
    std::vector<pass_parameter_pair> m_parameters;

    std::size_t m_ubo_count;
    std::size_t m_cis_count;
};

class vk_pass_parameter : public pass_parameter_interface
{
public:
    vk_pass_parameter(pass_parameter_layout_interface* layout);

    virtual void set(std::size_t index, bool value) override { upload_value(index, value); }
    virtual void set(std::size_t index, std::uint32_t value) override
    {
        upload_value(index, value);
    }
    virtual void set(std::size_t index, float value) override { upload_value(index, value); }
    virtual void set(std::size_t index, const math::float2& value) override
    {
        upload_value(index, value);
    }
    virtual void set(std::size_t index, const math::float3& value) override
    {
        upload_value(index, value);
    }
    virtual void set(std::size_t index, const math::float4& value) override
    {
        upload_value(index, value);
    }
    virtual void set(std::size_t index, const math::float4x4& value) override;
    virtual void set(std::size_t index, const math::float4x4* data, std::size_t size) override;
    virtual void set(std::size_t index, resource_interface* texture) override;

    void sync();

    VkDescriptorSet descriptor_set() const;

private:
    struct parameter_info
    {
        std::size_t offset;
        std::size_t size;
        pass_parameter_type type;
        std::size_t dirty;
        std::uint32_t binding;
    };

    template <typename T>
    void upload_value(std::size_t index, const T& value)
    {
        std::memcpy(m_cpu_buffer.data() + m_parameter_info[index].offset, &value, sizeof(T));
        mark_dirty(index);
    }

    void mark_dirty(std::size_t index);

    std::vector<VkDescriptorSet> m_descriptor_set;

    std::size_t m_dirty;
    std::size_t m_last_sync_frame;

    std::vector<parameter_info> m_parameter_info;

    std::vector<std::uint8_t> m_cpu_buffer;
    std::unique_ptr<vk_uniform_buffer> m_gpu_buffer;
    std::vector<vk_texture*> m_textures;
};

class vk_pass_layout : public pass_layout_interface
{
public:
    vk_pass_layout(const pass_layout_desc& desc);
    virtual ~vk_pass_layout();

    VkPipelineLayout pass_layout() const noexcept { return m_pass_layout; }

private:
    VkPipelineLayout m_pass_layout;
};

class vk_pipeline
{
public:
    vk_pipeline(const pass_desc& desc, VkRenderPass render_pass, std::size_t index);

    void begin(VkCommandBuffer command_buffer);
    void end(VkCommandBuffer command_buffer);

    VkPipelineLayout layout() const noexcept { return m_pass_layout->pass_layout(); }
    VkPipeline pipeline() const noexcept { return m_pipeline; }

private:
    VkShaderModule load_shader(std::string_view file);

    vk_pass_layout* m_pass_layout;
    VkPipeline m_pipeline;
};

class vk_render_pass : public technique_interface
{
public:
    vk_render_pass(const technique_desc& desc);
    ~vk_render_pass();

    void begin(VkCommandBuffer command_buffer, VkFramebuffer frame_buffer);
    void end(VkCommandBuffer command_buffer);
    void next(VkCommandBuffer command_buffer);

    VkRenderPass render_pass() const noexcept { return m_render_pass; }
    vk_pipeline& current_subpass() { return m_pipelines[m_subpass_index]; }

private:
    void create_pass(const technique_desc& desc);

    VkRenderPass m_render_pass;
    std::vector<vk_pipeline> m_pipelines;

    std::size_t m_subpass_index;
};
} // namespace ash::graphics::vk
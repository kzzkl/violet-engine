#pragma once

#include "vk_common.hpp"
#include "vk_resource.hpp"

namespace violet::graphics::vk
{
class vk_pipeline_parameter_layout : public pipeline_parameter_layout_interface
{
public:
    vk_pipeline_parameter_layout(const pipeline_parameter_layout_desc& desc);

    VkDescriptorSetLayout layout() const noexcept { return m_descriptor_set_layout; }
    const std::vector<pipeline_parameter_pair>& parameters() const noexcept { return m_parameters; }

    std::pair<std::size_t, std::size_t> descriptor_count() const noexcept
    {
        return {m_ubo_count, m_cis_count};
    }

private:
    VkDescriptorSetLayout m_descriptor_set_layout;
    std::vector<pipeline_parameter_pair> m_parameters;

    std::size_t m_ubo_count;
    std::size_t m_cis_count;
};

class vk_pipeline_parameter : public pipeline_parameter_interface
{
public:
    vk_pipeline_parameter(pipeline_parameter_layout_interface* layout);

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
        pipeline_parameter_type type;
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

class vk_pipeline
{
public:
    vk_pipeline(const pipeline_desc& desc, VkRenderPass render_pass, std::size_t index);

    void begin(VkCommandBuffer command_buffer);
    void end(VkCommandBuffer command_buffer);

    VkPipelineLayout layout() const noexcept { return m_pass_layout; }
    VkPipeline pipeline() const noexcept { return m_pipeline; }

private:
    VkShaderModule load_shader(std::string_view file);

    VkPipelineLayout m_pass_layout;
    VkPipeline m_pipeline;
};

struct vk_camera_info
{
    vk_image* render_target;
    vk_image* render_target_resolve;
    vk_image* depth_stencil_buffer;

    resource_extent extent() const
    {
        VIOLET_VK_ASSERT(
            render_target != nullptr || render_target_resolve != nullptr ||
            depth_stencil_buffer != nullptr);

        if (render_target != nullptr)
            return render_target->extent();
        else if (render_target_resolve != nullptr)
            return render_target_resolve->extent();
        else
            return depth_stencil_buffer->extent();
    }

    inline bool operator==(const vk_camera_info& other) const noexcept
    {
        return render_target == other.render_target &&
               render_target_resolve == other.render_target_resolve &&
               depth_stencil_buffer == other.depth_stencil_buffer;
    }
};

class vk_frame_buffer_layout
{
public:
    struct attachment_info
    {
        attachment_type type;
        VkAttachmentDescription description;
    };

public:
    vk_frame_buffer_layout(attachment_desc* attachment, std::size_t count);

    auto begin() noexcept { return m_attachments.begin(); }
    auto end() noexcept { return m_attachments.end(); }

    auto begin() const noexcept { return m_attachments.begin(); }
    auto end() const noexcept { return m_attachments.end(); }

    auto cbegin() const noexcept { return m_attachments.cbegin(); }
    auto cend() const noexcept { return m_attachments.cend(); }

private:
    std::vector<attachment_info> m_attachments;
};

class vk_render_pass;
class vk_frame_buffer
{
public:
    vk_frame_buffer(vk_render_pass* render_pass, const vk_camera_info& camera_info);
    vk_frame_buffer(vk_frame_buffer&& other);
    ~vk_frame_buffer();

    VkFramebuffer frame_buffer() const noexcept { return m_frame_buffer; }
    const std::vector<VkClearValue>& clear_values() const noexcept { return m_clear_values; }

    vk_frame_buffer& operator=(vk_frame_buffer&& other);

private:
    VkFramebuffer m_frame_buffer;

    std::vector<std::unique_ptr<vk_image>> m_attachments;
    std::vector<VkClearValue> m_clear_values;
};

class vk_frame_buffer_manager
{
public:
    vk_frame_buffer* get_or_create_frame_buffer(
        vk_render_pass* render_pass,
        const vk_camera_info& camera_info);

    void notify_destroy(vk_image* image);

private:
    struct vk_camera_info_hash
    {
        std::size_t operator()(const vk_camera_info& key) const
        {
            std::size_t result = 0;
            hviolet_combine(result, key.render_target->view());
            hviolet_combine(result, key.depth_stencil_buffer->view());
            hviolet_combine(
                result,
                key.render_target_resolve == nullptr ? VK_NULL_HANDLE
                                                     : key.render_target_resolve->view());

            return result;
        }

        template <class T>
        void hviolet_combine(std::size_t& s, const T& v) const
        {
            std::hash<T> h;
            s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
        }
    };

    std::unordered_map<vk_camera_info, std::unique_ptr<vk_frame_buffer>, vk_camera_info_hash>
        m_frame_buffers;
};

class vk_render_pass : public render_pass_interface
{
public:
    vk_render_pass(const render_pass_desc& desc);
    ~vk_render_pass();

    void begin(VkCommandBuffer command_buffer, const vk_camera_info& camera_info);
    void end(VkCommandBuffer command_buffer);
    void next(VkCommandBuffer command_buffer);

    VkRenderPass render_pass() const noexcept { return m_render_pass; }
    vk_pipeline& current_subpass() { return m_pipelines[m_subpass_index]; }

    vk_frame_buffer_layout& frame_buffer_layout() const { return *m_frame_buffer_layout; }

private:
    void create_pass(const render_pass_desc& desc);

    VkRenderPass m_render_pass;
    std::vector<vk_pipeline> m_pipelines;

    std::size_t m_subpass_index;

    std::unique_ptr<vk_frame_buffer_layout> m_frame_buffer_layout;
};
} // namespace violet::graphics::vk
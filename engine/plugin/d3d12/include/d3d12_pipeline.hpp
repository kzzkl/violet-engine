#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"
#include <string_view>
#include <unordered_map>

namespace ash::graphics::d3d12
{
class d3d12_pipeline_parameter_layout : public pipeline_parameter_layout_interface
{
public:
    d3d12_pipeline_parameter_layout(const pipeline_parameter_layout_desc& desc);

    inline std::size_t parameter_offset(std::size_t index) const
    {
        return m_parameters[index].offset;
    }
    inline std::size_t parameter_size(std::size_t index) const { return m_parameters[index].size; }
    inline pipeline_parameter_type parameter_type(std::size_t index) const
    {
        return m_parameters[index].type;
    }

    inline std::size_t parameter_count() const noexcept { return m_parameters.size(); }

    inline std::size_t cbv_count() const noexcept { return m_cbv_count; }
    inline std::size_t srv_count() const noexcept { return m_srv_count; }

    inline std::size_t constant_buffer_size() const noexcept { return m_constant_buffer_size; }

private:
    struct parameter_info
    {
        std::size_t offset;
        std::size_t size;
        pipeline_parameter_type type;
    };

    std::vector<parameter_info> m_parameters;

    std::size_t m_cbv_count;
    std::size_t m_srv_count;

    std::size_t m_constant_buffer_size;
};

enum class d3d12_parameter_tier_type
{
    TIER1, // Constant Buffer View
    TIER2  // Descriptor Table
};

class d3d12_pipeline_parameter : public pipeline_parameter_interface
{
public:
    struct tier1_info
    {
        D3D12_ROOT_PARAMETER_TYPE type;
        D3D12_GPU_VIRTUAL_ADDRESS address;
    };

    struct tier2_info
    {
        D3D12_CPU_DESCRIPTOR_HANDLE base_cpu_handle;
        D3D12_GPU_DESCRIPTOR_HANDLE base_gpu_handle;
        std::size_t size;
    };

public:
    d3d12_pipeline_parameter(pipeline_parameter_layout_interface* layout);

    virtual void set(std::size_t index, bool value) override;
    virtual void set(std::size_t index, std::uint32_t value) override;
    virtual void set(std::size_t index, float value) override;
    virtual void set(std::size_t index, const math::float2& value) override;
    virtual void set(std::size_t index, const math::float3& value) override;
    virtual void set(std::size_t index, const math::float4& value) override;
    virtual void set(std::size_t index, const math::float4x4& value) override;
    virtual void set(std::size_t index, const math::float4x4* data, size_t size) override;
    virtual void set(std::size_t index, resource* texture) override;

    void sync();

    inline d3d12_parameter_tier_type tier() const noexcept { return m_tier; }
    inline tier1_info tier1() const noexcept
    {
        return m_tier_info[d3d12_frame_counter::frame_resource_index()].tier1;
    }
    inline tier2_info tier2() const noexcept
    {
        return m_tier_info[d3d12_frame_counter::frame_resource_index()].tier2;
    }

private:
    union tier_info {
        tier1_info tier1;
        tier2_info tier2;
    };

    void mark_dirty(std::size_t index);

    d3d12_parameter_tier_type m_tier;
    std::vector<tier_info> m_tier_info;

    std::size_t m_dirty;
    std::size_t m_last_sync_frame;
    std::vector<std::size_t> m_dirty_counter;

    d3d12_pipeline_parameter_layout* m_layout;

    std::vector<std::uint8_t> m_cpu_buffer;
    std::vector<d3d12_resource*> m_textures;
    std::unique_ptr<d3d12_upload_buffer> m_gpu_buffer;
};

class d3d12_frame_buffer_layout
{
public:
    d3d12_frame_buffer_layout(attachment_desc* attachments, std::size_t count);

    auto begin() const { return m_attachments.begin(); }
    auto end() const { return m_attachments.end(); }

private:
    std::vector<attachment_desc> m_attachments;
};

class d3d12_render_pass;
class d3d12_frame_buffer
{
public:
    struct attachment_info
    {
        d3d12_resource* resource;
        D3D12_RESOURCE_STATES initial_state;
        D3D12_RESOURCE_STATES final_state;
    };

public:
    d3d12_frame_buffer(d3d12_render_pass* render_pass, d3d12_resource* render_target);

    void begin_render(D3D12GraphicsCommandList* command_list);
    void end_render(D3D12GraphicsCommandList* command_list);

    const std::vector<attachment_info>& attachments() const noexcept { return m_attachments; }

private:
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_render_targets;
    D3D12_CPU_DESCRIPTOR_HANDLE m_depth_stencil;

    std::vector<attachment_info> m_attachments;
    std::vector<std::unique_ptr<d3d12_resource>> m_attachment_container;
};

class d3d12_pipeline
{
public:
    d3d12_pipeline(const pipeline_desc& desc);

    void begin(D3D12GraphicsCommandList* command_list, d3d12_frame_buffer* frame_buffer);
    void end(D3D12GraphicsCommandList* command_list, bool final = false);
    inline D3D12PipelineState* pipeline_state() const noexcept { return m_pipeline_state.Get(); }

private:
    void initialize_vertex_layout(const pipeline_desc& desc);
    void initialize_pipeline_layout(const pipeline_desc& desc);
    void initialize_pipeline_state(const pipeline_desc& desc);

    d3d12_ptr<D3DBlob> load_shader(std::string_view file);

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_vertex_layout;

    d3d12_ptr<D3D12RootSignature> m_root_signature;
    d3d12_ptr<D3D12PipelineState> m_pipeline_state;

    d3d12_frame_buffer* m_current_frame_buffer;

    std::vector<std::size_t> m_color_indices;
    std::size_t m_depth_index;
    // first: resolve target, second: resolve source
    std::vector<std::pair<std::size_t, std::size_t>> m_resolve_indices;
};

class d3d12_render_pass : public render_pass_interface
{
public:
    d3d12_render_pass(const render_pass_desc& desc);

    void begin(D3D12GraphicsCommandList* command_list, d3d12_resource* render_target);
    void end(D3D12GraphicsCommandList* command_list);
    void next(D3D12GraphicsCommandList* command_list);

    const d3d12_frame_buffer_layout& frame_buffer_layout() const noexcept
    {
        return m_frame_buffer_layout;
    }

private:
    std::vector<std::unique_ptr<d3d12_pipeline>> m_pipelines;

    struct pipeline_info
    {
        std::unique_ptr<d3d12_pipeline> pipeline;
        D3D12_CPU_DESCRIPTOR_HANDLE render_target;
        std::size_t render_target_count;
        D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil;
    };

    std::size_t m_current_index;
    d3d12_frame_buffer* m_current_frame_buffer;

    d3d12_frame_buffer_layout m_frame_buffer_layout;
};

class d3d12_frame_buffer_manager
{
public:
    d3d12_frame_buffer* get_or_create_frame_buffer(
        d3d12_render_pass* render_pass,
        d3d12_resource* render_target);

    void notify_destroy(d3d12_resource* render_target);

private:
    using key_type = std::pair<d3d12_render_pass*, d3d12_resource*>;
    struct key_hash
    {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& pair) const
        {
            return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
        }
    };

    std::unordered_map<key_type, std::unique_ptr<d3d12_frame_buffer>, key_hash> m_frame_buffers;
};
} // namespace ash::graphics::d3d12
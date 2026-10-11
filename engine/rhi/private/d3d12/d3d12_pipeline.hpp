#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"
#include <string_view>
#include <unordered_map>

namespace violet::d3d12
{
class d3d12_pipeline_parameter_layout
{
public:
    d3d12_pipeline_parameter_layout(const pipeline_parameter_desc& desc);

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
    inline std::size_t uav_count() const noexcept { return m_uav_count; }
    inline std::size_t view_count() const noexcept
    {
        return m_cbv_count + m_srv_count + m_uav_count;
    }

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
    std::size_t m_uav_count;

    std::size_t m_constant_buffer_size;
};

enum class d3d12_parameter_tier_type
{
    TIER1, // Constant Buffer View
    TIER2  // Descriptor Table
};

class d3d12_pipeline_parameter : public rhi_pipeline_parameter
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
    d3d12_pipeline_parameter(const pipeline_parameter_desc& desc);
    virtual ~d3d12_pipeline_parameter();

    virtual void set(std::size_t index, const void* data, size_t size) override;
    virtual void set(std::size_t index, rhi_resource* texture, std::size_t offset) override;
    virtual void* get_constant_buffer_pointer(std::size_t index) override;

    inline d3d12_parameter_tier_type tier() const noexcept { return m_tier; }
    inline tier1_info tier1() const noexcept { return m_tier_info[m_current_index].tier1; }
    inline tier2_info tier2() const noexcept { return m_tier_info[m_current_index].tier2; }

private:
    union tier_info {
        tier1_info tier1;
        tier2_info tier2;
    };

    void sync_frame_resource();

    void copy_buffer(const void* data, std::size_t size, std::size_t offset);
    void copy_descriptor(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, std::size_t offset);

    d3d12_parameter_tier_type m_tier;
    std::vector<tier_info> m_tier_info;

    d3d12_pipeline_parameter_layout* m_layout;

    std::unique_ptr<d3d12_upload_buffer> m_gpu_buffer;
    std::vector<std::pair<D3D12_CPU_DESCRIPTOR_HANDLE, std::size_t>> m_views;
    std::size_t m_current_index;
    std::size_t m_last_sync_frame;
};

class d3d12_root_signature
{
public:
    d3d12_root_signature(
        const pipeline_parameter_desc* const parameters,
        std::size_t parameter_count);

    D3D12RootSignature* handle() const noexcept { return m_root_signature.Get(); }

private:
    d3d12_ptr<D3D12RootSignature> m_root_signature;
};

class d3d12_frame_buffer_layout
{
public:
    d3d12_frame_buffer_layout(const attachment_desc* attachments, std::size_t count);

    auto begin() const { return m_attachments.begin(); }
    auto end() const { return m_attachments.end(); }

private:
    std::vector<attachment_desc> m_attachments;
};

struct d3d12_camera_info
{
    d3d12_resource* render_target;
    d3d12_resource* render_target_resolve;
    d3d12_resource* depth_stencil_buffer;
};

class d3d12_render_pipeline;
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
    d3d12_frame_buffer(d3d12_render_pipeline* pipeline, const d3d12_camera_info& camera_info);

    void begin_render(D3D12GraphicsCommandList* command_list);
    void end_render(D3D12GraphicsCommandList* command_list);

    const std::vector<attachment_info>& attachments() const noexcept { return m_attachments; }

    resource_extent get_extent() const { return m_attachments[0].resource->get_extent(); }

private:
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_render_targets;
    D3D12_CPU_DESCRIPTOR_HANDLE m_depth_stencil;

    std::vector<attachment_info> m_attachments;
    std::vector<std::unique_ptr<d3d12_resource>> m_attachment_container;
};

class d3d12_render_pass
{
public:
    d3d12_render_pass(const render_pipeline_desc& desc, std::size_t index);

    void begin(D3D12GraphicsCommandList* command_list, d3d12_frame_buffer* frame_buffer);
    void end(D3D12GraphicsCommandList* command_list, bool final = false);
    inline D3D12PipelineState* pipeline_state() const noexcept { return m_pipeline_state.Get(); }

private:
    void initialize_vertex_layout(const render_pipeline_desc& desc, std::size_t index);
    void initialize_pipeline_state(const render_pipeline_desc& desc, std::size_t index);

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_vertex_layout;

    std::unique_ptr<d3d12_root_signature> m_root_signature;
    d3d12_ptr<D3D12PipelineState> m_pipeline_state;

    d3d12_frame_buffer* m_current_frame_buffer;

    std::vector<std::size_t> m_color_indexes;
    std::size_t m_depth_index;
    // first: resolve target, second: resolve source
    std::vector<std::pair<std::size_t, std::size_t>> m_resolve_indexes;
};

class d3d12_render_pipeline : public rhi_render_pipeline
{
public:
    d3d12_render_pipeline(const render_pipeline_desc& desc);

    void begin(D3D12GraphicsCommandList* command_list, const d3d12_camera_info& camera_info);
    void end(D3D12GraphicsCommandList* command_list);
    void next(D3D12GraphicsCommandList* command_list);

    const d3d12_frame_buffer_layout& frame_buffer_layout() const noexcept
    {
        return m_frame_buffer_layout;
    }

private:
    std::vector<std::unique_ptr<d3d12_render_pass>> m_passes;

    struct pass_info
    {
        std::unique_ptr<d3d12_render_pass> pass;
        D3D12_CPU_DESCRIPTOR_HANDLE render_target;
        std::size_t render_target_count;
        D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil;
    };

    std::size_t m_current_index;
    d3d12_frame_buffer* m_current_frame_buffer;

    d3d12_frame_buffer_layout m_frame_buffer_layout;
};

class d3d12_compute_pipeline : public compute_pipeline_interface
{
public:
    d3d12_compute_pipeline(const compute_pipeline_desc& desc);

    void begin(D3D12GraphicsCommandList* command_list);
    void end(D3D12GraphicsCommandList* command_list);

private:
    std::unique_ptr<d3d12_root_signature> m_root_signature;
    d3d12_ptr<D3D12PipelineState> m_pipeline_state;
};
} // namespace violet::d3d12
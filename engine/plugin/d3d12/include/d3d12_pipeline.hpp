#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"
#include <string_view>

namespace ash::graphics::d3d12
{
enum class d3d12_parameter_tier_type
{
    TIER1, // Constant Buffer View
    TIER2  // Descriptor Table
};

class d3d12_pass_parameter : public pass_parameter
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
    d3d12_pass_parameter(const pass_parameter_desc& desc);

    virtual void set(std::size_t index, bool value) override;
    virtual void set(std::size_t index, std::uint32_t value) override;
    virtual void set(std::size_t index, float value) override;
    virtual void set(std::size_t index, const math::float2& value) override;
    virtual void set(std::size_t index, const math::float3& value) override;
    virtual void set(std::size_t index, const math::float4& value) override;
    virtual void set(std::size_t index, const math::float4x4& value, bool row_matrix) override;
    virtual void set(std::size_t index, const math::float4x4* data, size_t size, bool row_matrix)
        override;
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
    struct parameter_info
    {
        std::size_t offset;
        std::size_t size;
        pass_parameter_type type;
        std::size_t dirty;
    };

    union tier_info {
        tier1_info tier1;
        tier2_info tier2;
    };

    void mark_dirty(std::size_t index);

    d3d12_parameter_tier_type m_tier;
    std::vector<tier_info> m_tier_info;

    std::size_t m_dirty;
    std::size_t m_last_sync_frame;

    std::vector<parameter_info> m_parameter_info;

    std::vector<std::uint8_t> m_cpu_buffer;
    std::vector<d3d12_resource*> m_textures;
    std::unique_ptr<d3d12_upload_buffer> m_gpu_buffer;
};

class d3d12_parameter_layout : public pass_layout
{
public:
    d3d12_parameter_layout(const pass_layout_desc& desc);
    inline D3D12RootSignature* root_signature() const noexcept { return m_root_signature.Get(); }

private:
    d3d12_ptr<D3D12RootSignature> m_root_signature;
};

class d3d12_pipeline : public pipeline
{
public:
    d3d12_pipeline(const pass_desc& desc);

    inline D3D12PipelineState* pass_state() const noexcept { return m_pass_state.Get(); }

private:
    void initialize_vertex_layout(const pass_desc& desc);
    void initialize_pass_state(const pass_desc& desc);

    d3d12_ptr<D3DBlob> load_shader(
        std::string_view entry,
        std::string_view target,
        std::string_view file);

    d3d12_ptr<D3D12GraphicsCommandList> m_command_list;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_vertex_layout;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE m_primitive_topology;

    d3d12_ptr<D3D12PipelineState> m_pass_state;

    d3d12_parameter_layout* m_parameter_layout;
};
} // namespace ash::graphics::d3d12
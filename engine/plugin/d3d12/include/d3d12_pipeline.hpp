#pragma once

#include "d3d12_common.hpp"
#include "d3d12_resource.hpp"
#include <string_view>

namespace ash::graphics::d3d12
{
enum class d3d12_parameter_tier_type
{
    TIER1,
    TIER2
};

class d3d12_pipeline_parameter : public pipeline_parameter
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

    virtual void bind(std::size_t index, resource* data, std::size_t offset) override;

    inline d3d12_parameter_tier_type tier() const noexcept { return m_tier; }
    inline tier1_info tier1() const noexcept { return m_tier1; }
    inline tier2_info tier2() const noexcept { return m_tier2; }

private:
    std::size_t m_descriptor_offset;

    d3d12_parameter_tier_type m_tier;
    std::vector<pipeline_parameter_type> m_type;

    union {
        tier1_info m_tier1;
        tier2_info m_tier2;
    };
};

class d3d12_parameter_layout : public pipeline_layout
{
public:
    d3d12_parameter_layout(const pipeline_layout_desc& desc);
    inline D3D12RootSignature* root_signature() const noexcept { return m_root_signature.Get(); }

private:
    d3d12_ptr<D3D12RootSignature> m_root_signature;
};

class d3d12_pipeline : public pipeline
{
public:
    d3d12_pipeline(const pipeline_desc& desc);

    inline D3D12PipelineState* pipeline_state() const noexcept { return m_pipeline_state.Get(); }

private:
    void initialize_vertex_layout(const pipeline_desc& desc);
    void initialize_pipeline_state(const pipeline_desc& desc);

    d3d12_ptr<D3DBlob> load_shader(
        std::string_view entry,
        std::string_view target,
        std::string_view file);

    d3d12_ptr<D3D12GraphicsCommandList> m_command_list;

    std::vector<D3D12_INPUT_ELEMENT_DESC> m_vertex_layout;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE m_primitive_topology;

    d3d12_ptr<D3D12PipelineState> m_pipeline_state;

    d3d12_parameter_layout* m_parameter_layout;
};
} // namespace ash::graphics::d3d12
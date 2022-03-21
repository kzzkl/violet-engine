#include "d3d12_pipeline.hpp"
#include "d3d12_context.hpp"
#include "d3d12_utility.hpp"
#include <d3dcompiler.h>
#include <string>

namespace ash::graphics::d3d12
{
d3d12_pipeline_parameter::d3d12_pipeline_parameter(const pipeline_parameter_desc& desc)
{
    if (desc.size == 1)
    {
        m_tier = d3d12_parameter_tier_type::TIER1;

        if (desc.data[0].type == pipeline_parameter_type::BUFFER)
            m_tier1.type = D3D12_ROOT_PARAMETER_TYPE_CBV;
        else if (desc.data[0].type == pipeline_parameter_type::TEXTURE)
            m_tier1.type = D3D12_ROOT_PARAMETER_TYPE_SRV;
    }
    else
    {
        auto heap =
            d3d12_context::resource()->visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        m_tier = d3d12_parameter_tier_type::TIER2;
        std::size_t offset = heap->allocate(desc.size);
        m_tier2.base_cpu_handle = heap->cpu_handle(offset);
        m_tier2.base_gpu_handle = heap->gpu_handle(offset);
        m_tier2.size = desc.size;

        for (std::size_t i = 0; i < desc.size; ++i)
            m_type.push_back(desc.data[i].type);
    }
}

void d3d12_pipeline_parameter::bind(std::size_t index, resource* data)
{
    d3d12_resource* d = static_cast<d3d12_resource*>(data);

    D3D12_GPU_VIRTUAL_ADDRESS address = d->resource()->GetGPUVirtualAddress();

    if (m_tier == d3d12_parameter_tier_type::TIER1)
    {
        m_tier1.address = address;
    }
    else
    {
        auto device = d3d12_context::device();
        auto heap =
            d3d12_context::resource()->visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        CD3DX12_CPU_DESCRIPTOR_HANDLE handle(m_tier2.base_cpu_handle, heap->increment_size());
        handle.Offset(static_cast<INT>(index));

        if (m_type[index] == pipeline_parameter_type::BUFFER)
        {
            D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
            desc.BufferLocation = address;
            desc.SizeInBytes = static_cast<UINT>(d->size());
            device->CreateConstantBufferView(&desc, handle);
        }
        else if (m_type[index] == pipeline_parameter_type::TEXTURE)
        {
            // TODO:
        }
    }
}

d3d12_parameter_layout::d3d12_parameter_layout(const pipeline_layout_desc& desc)
{
    std::vector<CD3DX12_ROOT_PARAMETER> root_parameter;
    root_parameter.resize(desc.size);

    UINT cbv_register_counter = 0;
    UINT srv_register_counter = 0;

    for (std::size_t i = 0; i < desc.size; ++i)
    {
        auto& parameter = desc.data[i];
        if (parameter.size == 1)
        {
            if (parameter.data[0].type == pipeline_parameter_type::BUFFER)
            {
                root_parameter[i].InitAsConstantBufferView(cbv_register_counter);
                ++cbv_register_counter;
            }
            else if (parameter.data[0].type == pipeline_parameter_type::TEXTURE)
            {
                root_parameter[i].InitAsShaderResourceView(srv_register_counter);
                ++srv_register_counter;
            }
        }
        else
        {
            UINT cbv_counter = 0;
            UINT srv_counter = 0;
            for (std::size_t j = 0; j < parameter.size; ++j)
            {
                auto& table = parameter.data[j];
                if (table.type == pipeline_parameter_type::BUFFER)
                    ++cbv_counter;
                else if (table.type == pipeline_parameter_type::TEXTURE)
                    ++srv_counter;
            }

            std::vector<CD3DX12_DESCRIPTOR_RANGE> range;
            if (cbv_counter != 0)
            {
                CD3DX12_DESCRIPTOR_RANGE cbv_range;
                cbv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, cbv_counter, cbv_register_counter);
                range.push_back(cbv_range);
                cbv_register_counter += cbv_counter;
            }
            if (srv_counter != 0)
            {
                CD3DX12_DESCRIPTOR_RANGE srv_range;
                srv_range.Init(
                    D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
                    srv_counter,
                    srv_register_counter,
                    0,
                    cbv_counter);
                range.push_back(srv_range);
                srv_register_counter += srv_counter;
            }

            root_parameter[i].InitAsDescriptorTable(static_cast<UINT>(range.size()), range.data());
        }
    }

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(
        static_cast<UINT>(root_parameter.size()),
        root_parameter.data(),
        0,
        nullptr,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    d3d12_ptr<D3DBlob> root_signature;
    d3d12_ptr<D3DBlob> error;
    HRESULT hr = D3D12SerializeRootSignature(
        &root_signature_desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &root_signature,
        &error);

    if (error != nullptr)
    {
        OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
    }
    else
    {
        throw_if_failed(d3d12_context::device()->CreateRootSignature(
            0,
            root_signature->GetBufferPointer(),
            root_signature->GetBufferSize(),
            IID_PPV_ARGS(&m_root_signature)));
    }
}

d3d12_pipeline::d3d12_pipeline(const pipeline_desc& desc)
{
    m_parameter_layout = static_cast<d3d12_parameter_layout*>(desc.layout);

    initialize_vertex_layout(desc);
    initialize_pipeline_state(desc);
}

void d3d12_pipeline::initialize_vertex_layout(const pipeline_desc& desc)
{
    auto get_format = [](vertex_attribute_type type) -> DXGI_FORMAT {
        switch (type)
        {
        case vertex_attribute_type::INT:
            return DXGI_FORMAT_R32_SINT;
        case vertex_attribute_type::INT2:
            return DXGI_FORMAT_R32G32_SINT;
        case vertex_attribute_type::INT3:
            return DXGI_FORMAT_R32G32B32_SINT;
        case vertex_attribute_type::INT4:
            return DXGI_FORMAT_R32G32B32A32_SINT;
        case vertex_attribute_type::UINT:
            return DXGI_FORMAT_R32_UINT;
        case vertex_attribute_type::UINT2:
            return DXGI_FORMAT_R32G32_UINT;
        case vertex_attribute_type::UINT3:
            return DXGI_FORMAT_R32G32B32_UINT;
        case vertex_attribute_type::UINT4:
            return DXGI_FORMAT_R32G32B32A32_UINT;
        case vertex_attribute_type::FLOAT:
            return DXGI_FORMAT_R32_FLOAT;
        case vertex_attribute_type::FLOAT2:
            return DXGI_FORMAT_R32G32_FLOAT;
        case vertex_attribute_type::FLOAT3:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case vertex_attribute_type::FLOAT4:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;
        default:
            return DXGI_FORMAT_UNKNOWN;
        };
    };

    for (std::size_t i = 0; i < desc.vertex_layout.size; ++i)
    {
        auto& attribute = desc.vertex_layout.data[i];
        D3D12_INPUT_ELEMENT_DESC desc = {attribute.name,
                                         attribute.index,
                                         get_format(attribute.type),
                                         0,
                                         D3D12_APPEND_ALIGNED_ELEMENT,
                                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                                         0};
        m_vertex_layout.push_back(desc);
    }
}

void d3d12_pipeline::initialize_pipeline_state(const pipeline_desc& desc)
{
    d3d12_ptr<D3DBlob> vs_blob = load_shader("vs_main", "vs_5_0", desc.vertex_shader);
    d3d12_ptr<D3DBlob> ps_blob = load_shader("ps_main", "ps_5_0", desc.pixel_shader);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
    pso_desc.InputLayout = {m_vertex_layout.data(), static_cast<UINT>(m_vertex_layout.size())};
    pso_desc.pRootSignature = m_parameter_layout->root_signature();
    pso_desc.VS = CD3DX12_SHADER_BYTECODE(vs_blob.Get());
    pso_desc.PS = CD3DX12_SHADER_BYTECODE(ps_blob.Get());
    pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    pso_desc.SampleMask = UINT_MAX;
    pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso_desc.SampleDesc.Count = 1;
    pso_desc.SampleDesc.Quality = 0;
    pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    throw_if_failed(d3d12_context::device()->CreateGraphicsPipelineState(
        &pso_desc,
        IID_PPV_ARGS(&m_pipeline_state)));
}

d3d12_ptr<D3DBlob> d3d12_pipeline::load_shader(
    std::string_view entry,
    std::string_view target,
    std::string_view file)
{
#ifndef NDEBUG
    // Enable better shader debugging with the graphics debugging tools.
    UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compile_flags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    d3d12_ptr<D3DBlob> result;
    d3d12_ptr<D3DBlob> error;
    D3DCompileFromFile(
        string_to_wstring(file).c_str(),
        nullptr,
        nullptr,
        entry.data(),
        target.data(),
        compile_flags,
        0,
        &result,
        &error);

    if (error != nullptr)
    {
        OutputDebugStringA(static_cast<char*>(error->GetBufferPointer()));
        return nullptr;
    }
    else
    {
        return result;
    }
}
} // namespace ash::graphics::d3d12
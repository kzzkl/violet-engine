#include "d3d12_pipeline.hpp"
#include "d3d12_context.hpp"
#include "d3d12_utility.hpp"
#include <d3dcompiler.h>
#include <string>

namespace ash::graphics::d3d12
{
static const std::vector<CD3DX12_STATIC_SAMPLER_DESC> static_samplers = {
    CD3DX12_STATIC_SAMPLER_DESC(
        0,                                // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP), // addressW

    CD3DX12_STATIC_SAMPLER_DESC(
        1,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP), // addressW

    CD3DX12_STATIC_SAMPLER_DESC(
        2,                                // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP), // addressW

    CD3DX12_STATIC_SAMPLER_DESC(
        3,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP), // addressW

    CD3DX12_STATIC_SAMPLER_DESC(
        4,                               // shaderRegister
        D3D12_FILTER_ANISOTROPIC,        // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
        0.0f,                            // mipLODBias
        8),                              // maxAnisotropy

    CD3DX12_STATIC_SAMPLER_DESC(
        5,                                // shaderRegister
        D3D12_FILTER_ANISOTROPIC,         // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
        0.0f,                             // mipLODBias
        8),                               // maxAnisotropy

    CD3DX12_STATIC_SAMPLER_DESC(
        6,                                                // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressW
        0.0f,                                             // mipLODBias
        16,                                               // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK)};

d3d12_pipeline_parameter::d3d12_pipeline_parameter(const pipeline_parameter_desc& desc)
    : m_dirty(0),
      m_last_sync_frame(-1),
      m_tier_info(d3d12_frame_counter::frame_resource_count())
{
    auto cal_align = [](std::size_t begin, std::size_t align) {
        return (begin + align - 1) & ~(align - 1);
    };

    std::size_t cbv_count = 0;
    std::size_t srv_count = 0;

    std::size_t constant_offset = 0;
    std::size_t texture_offset = 0;
    m_parameter_info.reserve(desc.size);
    for (std::size_t i = 0; i < desc.size; ++i)
    {
        std::size_t align_address = 0;
        std::size_t size = 0;
        switch (desc.data[i].type)
        {
        case pipeline_parameter_type::BOOL:
            align_address = cal_align(constant_offset, 4);
            size = sizeof(bool);
            break;
        case pipeline_parameter_type::UINT:
            align_address = cal_align(constant_offset, 4);
            size = sizeof(std::uint32_t);
            break;
        case pipeline_parameter_type::FLOAT:
            align_address = cal_align(constant_offset, 4);
            size = sizeof(float);
            break;
        case pipeline_parameter_type::FLOAT2:
            align_address = cal_align(constant_offset, 16);
            size = sizeof(math::float2);
            break;
        case pipeline_parameter_type::FLOAT3:
            align_address = cal_align(constant_offset, 16);
            size = sizeof(math::float3);
            break;
        case pipeline_parameter_type::FLOAT4:
            align_address = cal_align(constant_offset, 16);
            size = sizeof(math::float4);
            break;
        case pipeline_parameter_type::FLOAT4x4:
            align_address = cal_align(constant_offset, 16);
            size = sizeof(math::float4x4);
            break;
        case pipeline_parameter_type::FLOAT4x4_ARRAY:
            align_address = cal_align(constant_offset, 16);
            size = sizeof(math::float4x4) * desc.data[i].size;
            break;
        default:
            break;
        }

        if (desc.data[i].type == pipeline_parameter_type::TEXTURE)
        {
            ++srv_count;
            align_address = texture_offset;
            ++texture_offset;
        }
        else
        {
            cbv_count = 1;
            constant_offset = align_address + size;
        }

        m_parameter_info.push_back(parameter_info{align_address, size, desc.data[i].type, 0});
    }

    std::size_t buffer_size = cal_align(constant_offset, 256);
    if (cbv_count != 0)
    {
        m_cpu_buffer.resize(buffer_size);
        m_gpu_buffer = std::make_unique<d3d12_upload_buffer>(
            buffer_size * d3d12_frame_counter::frame_resource_count());
    }

    m_textures.resize(srv_count);

    if (srv_count != 0)
    {
        m_tier = d3d12_parameter_tier_type::TIER2;

        std::size_t view_count = cbv_count + srv_count;
        auto heap = d3d12_context::resource()->visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        std::size_t handle_offset =
            heap->allocate(view_count * d3d12_frame_counter::frame_resource_count());
        std::size_t gpu_buffer_offset = 0;
        for (auto& iter_info : m_tier_info)
        {
            iter_info.tier2.base_cpu_handle = heap->cpu_handle(handle_offset);
            iter_info.tier2.base_gpu_handle = heap->gpu_handle(handle_offset);
            iter_info.tier2.size = view_count;

            if (cbv_count != 0)
            {
                D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
                cbv_desc.BufferLocation =
                    m_gpu_buffer->resource()->GetGPUVirtualAddress() + gpu_buffer_offset;
                cbv_desc.SizeInBytes = static_cast<UINT>(buffer_size);
                d3d12_context::device()->CreateConstantBufferView(
                    &cbv_desc,
                    heap->cpu_handle(handle_offset + srv_count));
                gpu_buffer_offset += buffer_size;
            }

            handle_offset += view_count;
        }
    }
    else if (cbv_count != 0)
    {
        m_tier = d3d12_parameter_tier_type::TIER1;

        std::size_t gpu_buffer_offset = 0;
        for (auto& iter_info : m_tier_info)
        {
            iter_info.tier1.type = D3D12_ROOT_PARAMETER_TYPE_CBV;
            iter_info.tier1.address =
                m_gpu_buffer->resource()->GetGPUVirtualAddress() + gpu_buffer_offset;
            gpu_buffer_offset += buffer_size;
        }
    }
}

void d3d12_pipeline_parameter::set(std::size_t index, bool value)
{
    std::memcpy(m_cpu_buffer.data() + m_parameter_info[index].offset, &value, sizeof(bool));
    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(std::size_t index, std::uint32_t value)
{
    std::memcpy(
        m_cpu_buffer.data() + m_parameter_info[index].offset,
        &value,
        sizeof(std::uint32_t));
    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(std::size_t index, float value)
{
    std::memcpy(m_cpu_buffer.data() + m_parameter_info[index].offset, &value, sizeof(float));
    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(std::size_t index, const math::float2& value)
{
    std::memcpy(m_cpu_buffer.data() + m_parameter_info[index].offset, &value, sizeof(math::float2));
    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(std::size_t index, const math::float3& value)
{
    std::memcpy(m_cpu_buffer.data() + m_parameter_info[index].offset, &value, sizeof(math::float3));
    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(std::size_t index, const math::float4& value)
{
    std::memcpy(m_cpu_buffer.data() + m_parameter_info[index].offset, &value, sizeof(math::float4));
    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(std::size_t index, const math::float4x4& value, bool row_matrix)
{
    if (row_matrix)
    {
        math::float4x4 t;
        math::float4x4_simd m = math::simd::load(value);
        m = math::matrix_simd::transpose(m);
        math::simd::store(m, t);

        std::memcpy(
            m_cpu_buffer.data() + m_parameter_info[index].offset,
            &t,
            sizeof(math::float4x4));
    }
    else
    {
        std::memcpy(
            m_cpu_buffer.data() + m_parameter_info[index].offset,
            &value,
            sizeof(math::float4x4));
    }

    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(
    std::size_t index,
    const math::float4x4* data,
    size_t size,
    bool row_matrix)
{
    if (row_matrix)
    {
        math::float4x4 t;
        for (std::size_t i = 0; i < size; ++i)
        {
            math::float4x4_simd m = math::simd::load(data[i]);
            m = math::matrix_simd::transpose(m);
            math::simd::store(m, t);

            std::memcpy(
                m_cpu_buffer.data() + m_parameter_info[index].offset + sizeof(math::float4x4) * i,
                &t,
                sizeof(math::float4x4));
        }
    }
    else
    {
        std::memcpy(
            m_cpu_buffer.data() + m_parameter_info[index].offset,
            &data,
            size * sizeof(math::float4x4));
    }

    mark_dirty(index);
}

void d3d12_pipeline_parameter::set(std::size_t index, const resource* texture)
{
    m_textures[m_parameter_info[index].offset] = static_cast<const d3d12_resource*>(texture);
    mark_dirty(index);
}

void d3d12_pipeline_parameter::sync()
{
    if (m_last_sync_frame == d3d12_frame_counter::frame_counter())
        return;

    m_last_sync_frame = d3d12_frame_counter::frame_counter();

    if (m_dirty == 0)
        return;

    std::size_t resource_index = d3d12_frame_counter::frame_resource_index();
    for (parameter_info& info : m_parameter_info)
    {
        if (info.dirty == 0)
            continue;

        if (info.type == pipeline_parameter_type::TEXTURE)
        {
            auto device = d3d12_context::device();
            const d3d12_resource* texture = m_textures[info.offset];

            D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
            desc.Format = texture->resource()->GetDesc().Format;
            desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc.Texture2D.MostDetailedMip = 0;
            desc.Texture2D.MipLevels = texture->resource()->GetDesc().MipLevels;
            desc.Texture2D.ResourceMinLODClamp = 0.0f;

            auto heap =
                d3d12_context::resource()->visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            CD3DX12_CPU_DESCRIPTOR_HANDLE handle(
                m_tier_info[resource_index].tier2.base_cpu_handle,
                static_cast<INT>(info.offset),
                heap->increment_size());
            device->CreateShaderResourceView(texture->resource(), &desc, handle);
        }
        else
        {
            m_gpu_buffer->upload(
                m_cpu_buffer.data() + info.offset,
                info.size,
                m_cpu_buffer.size() * resource_index + info.offset);
        }

        --info.dirty;
        if (info.dirty == 0)
            --m_dirty;
    }
}

void d3d12_pipeline_parameter::mark_dirty(std::size_t index)
{
    if (m_parameter_info[index].dirty == 0)
        ++m_dirty;

    m_parameter_info[index].dirty = d3d12_frame_counter::frame_resource_count();
}

d3d12_parameter_layout::d3d12_parameter_layout(const pipeline_layout_desc& desc)
{
    std::vector<CD3DX12_ROOT_PARAMETER> root_parameter;
    root_parameter.resize(desc.size);

    std::vector<std::vector<CD3DX12_DESCRIPTOR_RANGE>> range;
    range.resize(desc.size);

    UINT cbv_register_counter = 0;
    UINT srv_register_counter = 0;
    for (std::size_t i = 0; i < desc.size; ++i)
    {
        auto& parameter = desc.data[i];

        UINT cbv_counter = 0;
        UINT srv_counter = 0;

        for (std::size_t j = 0; j < parameter.size; ++j)
        {
            if (parameter.data[j].type == pipeline_parameter_type::TEXTURE)
                ++srv_counter;
            else
                cbv_counter = 1;
        }

        if (cbv_counter == 1 && srv_counter == 0)
        {
            root_parameter[i].InitAsConstantBufferView(cbv_register_counter);
            ++cbv_register_counter;
        }
        else
        {
            if (srv_counter != 0)
            {
                CD3DX12_DESCRIPTOR_RANGE srv_range;
                srv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, srv_counter, srv_register_counter);
                range[i].push_back(srv_range);
                srv_register_counter += srv_counter;
            }

            if (cbv_counter != 0)
            {
                CD3DX12_DESCRIPTOR_RANGE cbv_range;
                cbv_range.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, cbv_counter, cbv_register_counter);
                range[i].push_back(cbv_range);
                cbv_register_counter += cbv_counter;
            }

            root_parameter[i].InitAsDescriptorTable(
                static_cast<UINT>(range[i].size()),
                range[i].data());
        }
    }

    CD3DX12_ROOT_SIGNATURE_DESC root_signature_desc(
        static_cast<UINT>(root_parameter.size()),
        root_parameter.data(),
        static_cast<UINT>(static_samplers.size()),
        static_samplers.data(),
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
        D3D12_INPUT_ELEMENT_DESC desc = {
            attribute.name,
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
    pso_desc.NumRenderTargets = 1;
    pso_desc.RTVFormats[0] = d3d12_context::renderer()->render_target_format();
    pso_desc.SampleDesc = d3d12_context::renderer()->render_target_sample_desc();
    pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    if (desc.primitive_topology == primitive_topology_type::TRIANGLE_LIST)
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    else if (desc.primitive_topology == primitive_topology_type::LINE_LIST)
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

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
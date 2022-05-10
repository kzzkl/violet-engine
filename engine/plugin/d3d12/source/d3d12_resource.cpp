#include "d3d12_resource.hpp"
#include "DDSTextureLoader12.h"
#include "d3d12_context.hpp"
#include <fstream>

namespace ash::graphics::d3d12
{
d3d12_render_target_proxy::d3d12_render_target_proxy(d3d12_render_target* resource)
    : m_resource(resource)
{
}

void d3d12_render_target_proxy::begin_render(D3D12GraphicsCommandList* command_list)
{
    m_resource->begin_render(command_list);
}

void d3d12_render_target_proxy::end_render(D3D12GraphicsCommandList* command_list)
{
    m_resource->end_render(command_list);
}

void d3d12_render_target_proxy::resolve(D3D12Resource* target)
{
    m_resource->resolve(target);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_render_target_proxy::cpu_handle() const noexcept
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return heap->cpu_handle(m_resource->m_rtv_offset);
}

d3d12_depth_stencil_buffer_proxy::d3d12_depth_stencil_buffer_proxy(
    d3d12_depth_stencil_buffer* resource)
    : m_resource(resource)
{
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_depth_stencil_buffer_proxy::cpu_handle() const noexcept
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    return heap->cpu_handle(m_resource->m_dsv_offset);
}

d3d12_render_target_proxy d3d12_resource::render_target()
{
    throw d3d12_exception("The resource is not a render target");
}

d3d12_depth_stencil_buffer_proxy d3d12_resource::depth_stencil_buffer()
{
    throw d3d12_exception("The resource is not a depth stencil buffer");
}

d3d12_shader_resource_proxy d3d12_resource::shader_resource()
{
    throw d3d12_exception("The resource is not a shader resource");
}

d3d12_vertex_buffer_proxy d3d12_resource::vertex_buffer()
{
    throw d3d12_exception("The resource is not a vertex buffer");
}

d3d12_index_buffer_proxy d3d12_resource::index_buffer()
{
    throw d3d12_exception("The resource is not a index buffer");
}

d3d12_render_target::d3d12_render_target(
    std::uint32_t width,
    std::uint32_t height,
    std::size_t samples,
    resource_format format)
{
    auto device = d3d12_context::device();

    // Query sample level.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS sample_level = {};
    sample_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    sample_level.Format = to_d3d12_format(format);
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = static_cast<UINT>(samples);
    throw_if_failed(device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    CD3DX12_RESOURCE_DESC render_target_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        to_d3d12_format(format),
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,
        1,
        sample_level.SampleCount,
        sample_level.NumQualityLevels - 1,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = to_d3d12_format(format);
    clear.Color[0] = clear.Color[1] = clear.Color[2] = 0.0f;
    clear.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &render_target_desc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clear,
        IID_PPV_ARGS(&m_resource));
    resource_state(D3D12_RESOURCE_STATE_RENDER_TARGET);

    // Create RTV.
    auto rtv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_rtv_offset = rtv_heap->allocate(1);
    device->CreateRenderTargetView(m_resource.Get(), nullptr, rtv_heap->cpu_handle(m_rtv_offset));

    // Create SRV.
    auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srv_offset = srv_heap->allocate(1);
    device->CreateShaderResourceView(m_resource.Get(), nullptr, srv_heap->cpu_handle(m_srv_offset));
}

d3d12_render_target::d3d12_render_target(const render_target_desc& desc)
    : d3d12_render_target(desc.width, desc.height, desc.samples, desc.format)
{
}

d3d12_render_target::d3d12_render_target(d3d12_ptr<D3D12Resource> resource) : m_resource(resource)
{
    auto device = d3d12_context::device();
    resource_state(D3D12_RESOURCE_STATE_PRESENT);

    // Create RTV.
    auto rtv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_rtv_offset = rtv_heap->allocate(1);
    device->CreateRenderTargetView(m_resource.Get(), nullptr, rtv_heap->cpu_handle(m_rtv_offset));

    // Create SRV.
    auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srv_offset = srv_heap->allocate(1);
    device->CreateShaderResourceView(m_resource.Get(), nullptr, srv_heap->cpu_handle(m_srv_offset));
}

d3d12_render_target_proxy d3d12_render_target::render_target()
{
    return d3d12_render_target_proxy(this);
}

d3d12_shader_resource_proxy d3d12_render_target::shader_resource()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return d3d12_shader_resource_proxy(heap->cpu_handle(m_srv_offset));
}

resource_format d3d12_render_target::format() const noexcept
{
    return to_ash_format(m_resource->GetDesc().Format);
}

resource_extent d3d12_render_target::extent() const noexcept
{
    auto desc = m_resource->GetDesc();
    return resource_extent{
        static_cast<std::uint32_t>(desc.Width),
        static_cast<std::uint32_t>(desc.Height)};
}

void d3d12_render_target::begin_render(D3D12GraphicsCommandList* command_list)
{
}

void d3d12_render_target::end_render(D3D12GraphicsCommandList* command_list)
{
}

void d3d12_render_target::resolve(D3D12Resource* target)
{
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(
    std::uint32_t width,
    std::uint32_t height,
    std::size_t samples,
    resource_format format)
{
    auto device = d3d12_context::device();

    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_DEFAULT);

    // Query sample level.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS sample_level = {};
    sample_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    sample_level.Format = to_d3d12_format(format);
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = static_cast<UINT>(samples);
    throw_if_failed(device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    CD3DX12_RESOURCE_DESC depth_stencil_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        to_d3d12_format(format),
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,
        1,
        sample_level.SampleCount,
        sample_level.NumQualityLevels - 1,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = to_d3d12_format(format);
    clear.DepthStencil.Depth = 1.0f;
    clear.DepthStencil.Stencil = 0;

    throw_if_failed(device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &depth_stencil_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clear,
        IID_PPV_ARGS(&m_resource)));
    resource_state(D3D12_RESOURCE_STATE_DEPTH_WRITE);

    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_dsv_offset = heap->allocate(1);
    device->CreateDepthStencilView(m_resource.Get(), nullptr, heap->cpu_handle(m_dsv_offset));
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(const depth_stencil_buffer_desc& desc)
    : d3d12_depth_stencil_buffer(desc.width, desc.height, desc.samples, desc.format)
{
}

d3d12_depth_stencil_buffer_proxy d3d12_depth_stencil_buffer::depth_stencil_buffer()
{
    return d3d12_depth_stencil_buffer_proxy(this);
}

resource_format d3d12_depth_stencil_buffer::format() const noexcept
{
    return to_ash_format(m_resource->GetDesc().Format);
}

resource_extent d3d12_depth_stencil_buffer::extent() const noexcept
{
    auto desc = m_resource->GetDesc();
    return resource_extent{
        static_cast<std::uint32_t>(desc.Width),
        static_cast<std::uint32_t>(desc.Height)};
}

d3d12_texture::d3d12_texture(const char* file, D3D12GraphicsCommandList* command_list)
{
    std::ifstream fin(file, std::ios::in | std::ios::binary);
    if (!fin)
        throw d3d12_exception("Unable to open texture file.");

    std::vector<uint8_t> dds_data(fin.seekg(0, std::ios::end).tellg());
    fin.seekg(0, std::ios::beg).read((char*)dds_data.data(), dds_data.size());
    fin.close();

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    throw_if_failed(DirectX::LoadDDSTextureFromMemory(
        d3d12_context::device(),
        dds_data.data(),
        dds_data.size(),
        &m_resource,
        subresources));

    const UINT64 upload_resource_size =
        GetRequiredIntermediateSize(m_resource.Get(), 0, static_cast<UINT>(subresources.size()));

    d3d12_ptr<ID3D12Resource> upload_resource;
    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(upload_resource_size);
    throw_if_failed(d3d12_context::device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload_resource)));

    UpdateSubresources(
        command_list,
        m_resource.Get(),
        upload_resource.Get(),
        0,
        0,
        static_cast<UINT>(subresources.size()),
        subresources.data());

    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    command_list->ResourceBarrier(1, &transition);
    resource_state(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    d3d12_context::resource()->push_temporary_resource(upload_resource);

    // Create SRV.
    auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srv_offset = srv_heap->allocate(1);

    d3d12_context::device()->CreateShaderResourceView(
        m_resource.Get(),
        nullptr,
        srv_heap->cpu_handle(m_srv_offset));
}

d3d12_shader_resource_proxy d3d12_texture::shader_resource()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return d3d12_shader_resource_proxy(heap->cpu_handle(m_srv_offset));
}

d3d12_default_buffer::d3d12_default_buffer(
    const void* data,
    std::size_t size,
    D3D12GraphicsCommandList* command_list)
{
    auto device = d3d12_context::device();

    // Create intermediate upload heap.
    CD3DX12_HEAP_PROPERTIES upload_heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC upload_desc = CD3DX12_RESOURCE_DESC::Buffer(size);

    d3d12_ptr<D3D12Resource> upload_resource;
    throw_if_failed(device->CreateCommittedResource(
        &upload_heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &upload_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload_resource)));

    // Create default heap
    CD3DX12_HEAP_PROPERTIES default_heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC default_desc = CD3DX12_RESOURCE_DESC::Buffer(size);

    throw_if_failed(device->CreateCommittedResource(
        &default_heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &default_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_resource)));

    // Copy data.
    D3D12_SUBRESOURCE_DATA subresource = {};
    subresource.pData = data;
    subresource.RowPitch = size;
    subresource.SlicePitch = size;
    UpdateSubresources(
        command_list,
        m_resource.Get(),
        upload_resource.Get(),
        0,
        0,
        1,
        &subresource);

    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    command_list->ResourceBarrier(1, &transition);
    resource_state(D3D12_RESOURCE_STATE_GENERIC_READ);

    d3d12_context::resource()->push_temporary_resource(upload_resource);
}

d3d12_upload_buffer::d3d12_upload_buffer(
    const void* data,
    std::size_t size,
    D3D12GraphicsCommandList* command_list)
{
    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size);

    throw_if_failed(d3d12_context::device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)));
    resource_state(D3D12_RESOURCE_STATE_GENERIC_READ);

    if (data != nullptr && size != 0)
        upload(data, size);
}

void d3d12_upload_buffer::upload(const void* data, std::size_t size, std::size_t offset)
{
    void* mapped;
    throw_if_failed(m_resource->Map(0, nullptr, &mapped));

    void* target = static_cast<std::uint8_t*>(mapped) + offset;
    memcpy(target, data, size);

    m_resource->Unmap(0, nullptr);
}

d3d12_descriptor_heap::d3d12_descriptor_heap(
    D3D12_DESCRIPTOR_HEAP_TYPE type,
    std::size_t size,
    std::size_t increment_size,
    D3D12_DESCRIPTOR_HEAP_FLAGS flag)
    : m_increment_size(static_cast<UINT>(increment_size))
{
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.NumDescriptors = static_cast<UINT>(size);
    desc.Type = type;
    desc.Flags = flag;
    desc.NodeMask = 0;
    throw_if_failed(d3d12_context::device()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_heap)));
}

std::size_t d3d12_descriptor_heap::allocate(std::size_t size)
{
    return m_index_allocator.allocate(size);
}

void d3d12_descriptor_heap::deallocate(std::size_t begin, std::size_t size)
{
    m_index_allocator.deallocate(begin, size);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_descriptor_heap::cpu_handle(std::size_t index)
{
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(
        m_heap->GetCPUDescriptorHandleForHeapStart(),
        static_cast<UINT>(index),
        m_increment_size);
}

D3D12_GPU_DESCRIPTOR_HANDLE d3d12_descriptor_heap::gpu_handle(std::size_t index)
{
    return CD3DX12_GPU_DESCRIPTOR_HANDLE(
        m_heap->GetGPUDescriptorHandleForHeapStart(),
        static_cast<UINT>(index),
        m_increment_size);
}

d3d12_resource_manager::d3d12_resource_manager()
{
    auto device = d3d12_context::device();

    // Get the descriptor size.
    std::array<UINT, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES> descriptor_size;
    descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
    descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] =
        device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = std::make_unique<d3d12_descriptor_heap>(
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        1024,
        descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV],
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    m_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = std::make_unique<d3d12_descriptor_heap>(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        1024,
        descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_RTV],
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE);
    m_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = std::make_unique<d3d12_descriptor_heap>(
        D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
        512,
        descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_DSV],
        D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    m_visible_heaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] =
        std::make_unique<d3d12_descriptor_heap>(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
            1024,
            descriptor_size[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV],
            D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE);
}

void d3d12_resource_manager::push_temporary_resource(d3d12_ptr<D3D12Resource> resource)
{
    m_temporary.get().push_back(resource);
}

void d3d12_resource_manager::switch_frame_resources()
{
    m_temporary.get().clear();
}
} // namespace ash::graphics::d3d12
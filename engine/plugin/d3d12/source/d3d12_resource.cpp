#include "d3d12_resource.hpp"
#include "DDSTextureLoader12.h"
#include "d3d12_context.hpp"

namespace ash::graphics::d3d12
{
d3d12_render_target_proxy::d3d12_render_target_proxy(
    d3d12_render_target_base* resource,
    D3D12Resource* render_target,
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle)
    : m_resource(resource),
      m_render_target(render_target),
      m_rtv_handle(rtv_handle)
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

d3d12_render_target_proxy d3d12_resource::render_target()
{
    ASH_D3D12_ASSERT(
        false,
        "Conversion to render target is not supported for the current resource.");
    static const D3D12_CPU_DESCRIPTOR_HANDLE invalid;
    return d3d12_render_target_proxy(nullptr, nullptr, invalid);
}

d3d12_depth_stencil_proxy d3d12_resource::depth_stencil()
{
    ASH_D3D12_ASSERT(
        false,
        "Conversion to depth stencil is not supported for the current resource.");
    static const D3D12_CPU_DESCRIPTOR_HANDLE invalid;
    return d3d12_depth_stencil_proxy(invalid);
}

d3d12_vertex_buffer_proxy d3d12_resource::vertex_buffer()
{
    ASH_D3D12_ASSERT(
        false,
        "Conversion to vertex buffer is not supported for the current resource.");
    static const D3D12_VERTEX_BUFFER_VIEW invalid = {};
    return d3d12_vertex_buffer_proxy(invalid);
}

d3d12_index_buffer_proxy d3d12_resource::index_buffer()
{
    ASH_D3D12_ASSERT(
        false,
        "Conversion to index buffer is not supported for the current resource.");
    static const D3D12_INDEX_BUFFER_VIEW invalid = {};
    return d3d12_index_buffer_proxy(invalid, 0);
}

d3d12_shader_resource_proxy d3d12_resource::shader_resource()
{
    ASH_D3D12_ASSERT(
        false,
        "Conversion to shader resource is not supported for the current resource.");
    static const D3D12_CPU_DESCRIPTOR_HANDLE invalid = {};
    return d3d12_shader_resource_proxy(invalid);
}

d3d12_render_target::d3d12_render_target(
    std::uint32_t width,
    std::uint32_t height,
    DXGI_FORMAT format,
    D3D12_RESOURCE_STATES end_state)
{
    auto device = d3d12_context::device();

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = format;
    clear.Color[0] = clear.Color[1] = clear.Color[2] = 0.0f;
    clear.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        m_end_state,
        &clear,
        IID_PPV_ARGS(&m_render_target));

    // Create RTV.
    auto rtv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_rtv_offset = rtv_heap->allocate(1);
    device->CreateRenderTargetView(
        m_render_target.Get(),
        nullptr,
        rtv_heap->cpu_handle(m_rtv_offset));

    // Create SRV.
    auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srv_offset = srv_heap->allocate(1);
    device->CreateShaderResourceView(
        m_render_target.Get(),
        nullptr,
        srv_heap->cpu_handle(m_srv_offset));
}

d3d12_render_target::d3d12_render_target(d3d12_ptr<D3D12Resource> resource)
    : m_render_target(resource),
      m_rtv_offset(INVALID_DESCRIPTOR_INDEX),
      m_srv_offset(INVALID_DESCRIPTOR_INDEX)
{
}

d3d12_render_target::d3d12_render_target(d3d12_render_target&& other) noexcept
    : m_render_target(std::move(other.m_render_target)),
      m_rtv_offset(other.m_rtv_offset),
      m_srv_offset(other.m_srv_offset)
{
    other.m_rtv_offset = INVALID_DESCRIPTOR_INDEX;
    other.m_srv_offset = INVALID_DESCRIPTOR_INDEX;
}

d3d12_render_target::~d3d12_render_target()
{
    if (m_rtv_offset != INVALID_DESCRIPTOR_INDEX)
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        heap->deallocate(m_rtv_offset);
    }

    if (m_srv_offset != INVALID_DESCRIPTOR_INDEX)
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        heap->deallocate(m_srv_offset);
    }
}

void d3d12_render_target::begin_render(D3D12GraphicsCommandList* command_list)
{
    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_render_target.Get(),
        m_end_state,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    command_list->ResourceBarrier(1, &transition);
}

void d3d12_render_target::end_render(D3D12GraphicsCommandList* command_list)
{
    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_render_target.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        m_end_state);
    command_list->ResourceBarrier(1, &transition);
}

d3d12_render_target_proxy d3d12_render_target::render_target()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return d3d12_render_target_proxy(this, m_render_target.Get(), heap->cpu_handle(m_rtv_offset));
}

d3d12_shader_resource_proxy d3d12_render_target::shader_resource()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return d3d12_shader_resource_proxy(heap->cpu_handle(m_srv_offset));
}

d3d12_render_target& d3d12_render_target::operator=(d3d12_render_target&& other) noexcept
{
    if (this != &other)
    {
        d3d12_resource::operator=(std::move(other));
        m_rtv_offset = other.m_rtv_offset;
        other.m_rtv_offset = INVALID_DESCRIPTOR_INDEX;
    }

    return *this;
}

d3d12_render_target_mutlisample::d3d12_render_target_mutlisample(
    std::uint32_t width,
    std::uint32_t height,
    DXGI_FORMAT format,
    D3D12_RESOURCE_STATES end_state,
    std::size_t multiple_sampling,
    bool create_resolve_target)
    : m_end_state(end_state)
{
    auto device = d3d12_context::device();

    // Query sample level.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS sample_level = {};
    sample_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    sample_level.Format = format;
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = static_cast<UINT>(multiple_sampling);
    throw_if_failed(device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    desc.SampleDesc.Count = sample_level.SampleCount;
    desc.SampleDesc.Quality = sample_level.NumQualityLevels - 1;

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = format;
    clear.Color[0] = clear.Color[1] = clear.Color[2] = 0.0f;
    clear.Color[3] = 1.0f;

    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        &clear,
        IID_PPV_ARGS(&m_render_target));

    // Create RTV.
    auto rtv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_rtv_offset = rtv_heap->allocate(1);
    device->CreateRenderTargetView(
        m_render_target.Get(),
        nullptr,
        rtv_heap->cpu_handle(m_rtv_offset));

    // The shader resource for multisampled textures is not m_render_target, so multisampling does
    // not create SRVs here.
    if (multiple_sampling == 1)
    {
        // Create SRV.
        auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_srv_offset = srv_heap->allocate(1);
        device->CreateShaderResourceView(
            m_render_target.Get(),
            nullptr,
            srv_heap->cpu_handle(m_srv_offset));
    }
    else
    {
        m_srv_offset = INVALID_DESCRIPTOR_INDEX;
    }

    if (create_resolve_target)
    {
        auto device = d3d12_context::device();

        D3D12_RESOURCE_DESC desc = {};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Alignment = 0;
        desc.Width = static_cast<UINT>(width);
        desc.Height = static_cast<UINT>(height);
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Format = format;
        desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;

        D3D12_CLEAR_VALUE clear = {};
        clear.Format = format;
        clear.Color[0] = clear.Color[1] = clear.Color[2] = 0.0f;
        clear.Color[3] = 1.0f;

        CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_DEFAULT);
        device->CreateCommittedResource(
            &heap_properties,
            D3D12_HEAP_FLAG_NONE,
            &desc,
            m_end_state,
            &clear,
            IID_PPV_ARGS(&m_resolve_target));

        // Create SRV.
        auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_srv_offset = srv_heap->allocate(1);
        device->CreateShaderResourceView(
            m_resolve_target.Get(),
            nullptr,
            srv_heap->cpu_handle(m_srv_offset));
    }
}

d3d12_render_target_proxy d3d12_render_target_mutlisample::render_target()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return d3d12_render_target_proxy(this, m_render_target.Get(), heap->cpu_handle(m_rtv_offset));
}

d3d12_shader_resource_proxy d3d12_render_target_mutlisample::shader_resource()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return d3d12_shader_resource_proxy(heap->cpu_handle(m_srv_offset));
}

void d3d12_render_target_mutlisample::begin_render(D3D12GraphicsCommandList* command_list)
{
    D3D12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_render_target.Get(),
        D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    command_list->ResourceBarrier(1, &transition);
}

void d3d12_render_target_mutlisample::end_render(D3D12GraphicsCommandList* command_list)
{
    D3D12_RESOURCE_BARRIER copy_transition[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_render_target.Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_RESOLVE_SOURCE),
        CD3DX12_RESOURCE_BARRIER::Transition(
            m_resolve_target.Get(),
            m_end_state,
            D3D12_RESOURCE_STATE_RESOLVE_DEST)};

    command_list->ResourceBarrier(2, copy_transition);

    command_list->ResolveSubresource(
        m_resolve_target.Get(),
        0,
        m_render_target.Get(),
        0,
        m_render_target->GetDesc().Format);

    D3D12_RESOURCE_BARRIER resolve_transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_resolve_target.Get(),
        D3D12_RESOURCE_STATE_RESOLVE_DEST,
        m_end_state);
    command_list->ResourceBarrier(1, &resolve_transition);
}

void d3d12_render_target_mutlisample::bind_resolve(d3d12_ptr<D3D12Resource> resolve_target)
{
    m_resolve_target = resolve_target;
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer() noexcept
    : m_descriptor_offset(INVALID_DESCRIPTOR_INDEX)
{
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(
    std::uint32_t width,
    std::uint32_t height,
    DXGI_FORMAT format,
    std::size_t multiple_sampling)
{
    auto device = d3d12_context::device();

    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_DEFAULT);

    // Query sample level.
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS sample_level = {};
    sample_level.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    sample_level.Format = format;
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = static_cast<UINT>(multiple_sampling);
    throw_if_failed(device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    desc.Alignment = 0;
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = format;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    desc.SampleDesc.Count = sample_level.SampleCount;
    desc.SampleDesc.Quality = sample_level.NumQualityLevels - 1;

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = format;
    clear.DepthStencil.Depth = 1.0f;
    clear.DepthStencil.Stencil = 0;

    throw_if_failed(device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clear,
        IID_PPV_ARGS(&m_resource)));
    m_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_descriptor_offset = heap->allocate(1);
    device->CreateDepthStencilView(
        m_resource.Get(),
        nullptr,
        heap->cpu_handle(m_descriptor_offset));
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(d3d12_depth_stencil_buffer&& other) noexcept
    : m_resource(std::move(other.m_resource)),
      m_state(other.m_state),
      m_descriptor_offset(other.m_descriptor_offset)
{
    other.m_descriptor_offset = INVALID_DESCRIPTOR_INDEX;
}

d3d12_depth_stencil_buffer::~d3d12_depth_stencil_buffer()
{
    if (m_descriptor_offset != INVALID_DESCRIPTOR_INDEX)
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        heap->deallocate(m_descriptor_offset);
    }
}

d3d12_depth_stencil_proxy d3d12_depth_stencil_buffer::depth_stencil()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    return d3d12_depth_stencil_proxy(heap->cpu_handle(m_descriptor_offset));
}

d3d12_depth_stencil_buffer& d3d12_depth_stencil_buffer::operator=(
    d3d12_depth_stencil_buffer&& other) noexcept
{
    if (this != &other)
    {
        d3d12_resource::operator=(std::move(other));
        m_descriptor_offset = other.m_descriptor_offset;
        other.m_descriptor_offset = INVALID_DESCRIPTOR_INDEX;
    }

    return *this;
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
    m_state = D3D12_RESOURCE_STATE_GENERIC_READ;

    d3d12_context::resource()->push_temporary_resource(upload_resource);
}

d3d12_upload_buffer::d3d12_upload_buffer(
    const void* data,
    std::size_t size,
    D3D12GraphicsCommandList*)
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
    m_state = D3D12_RESOURCE_STATE_GENERIC_READ;

    throw_if_failed(m_resource->Map(0, nullptr, &m_mapped));

    if (data != nullptr)
        upload(data, size, 0);
}

d3d12_upload_buffer::d3d12_upload_buffer(d3d12_upload_buffer&& other) noexcept
    : m_resource(std::move(other.m_resource)),
      m_state(other.m_state),
      m_mapped(other.m_mapped)
{
    other.m_mapped = nullptr;
}

d3d12_upload_buffer::~d3d12_upload_buffer()
{
    if (m_mapped != nullptr)
    {
        m_resource->Unmap(0, nullptr);
        m_mapped = nullptr;
    }
}

void d3d12_upload_buffer::upload(const void* data, std::size_t size, std::size_t offset)
{
    void* target = static_cast<std::uint8_t*>(m_mapped) + offset;
    memcpy(target, data, size);
}

d3d12_upload_buffer& d3d12_upload_buffer::operator=(d3d12_upload_buffer&& other) noexcept
{
    if (this != &other)
    {
        d3d12_resource::operator=(std::move(other));
        m_mapped = other.m_mapped;
        other.m_mapped = nullptr;
    }

    return *this;
}

d3d12_texture::d3d12_texture(
    const std::uint8_t* data,
    std::size_t size,
    D3D12GraphicsCommandList* command_list)
{
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    throw_if_failed(DirectX::LoadDDSTextureFromMemory(
        d3d12_context::device(),
        data,
        size,
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
    m_state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    d3d12_context::resource()->push_temporary_resource(upload_resource);
}

d3d12_texture::d3d12_texture(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    D3D12GraphicsCommandList* command_list)
{
    auto device = d3d12_context::device();

    // Create default buffer.
    CD3DX12_HEAP_PROPERTIES default_heap_properties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC default_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_R8G8B8A8_UNORM,
        static_cast<UINT>(width),
        static_cast<UINT>(height));
    throw_if_failed(device->CreateCommittedResource(
        &default_heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &default_desc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&m_resource)));

    // Create upload buffer.
    UINT width_pitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u) &
                       ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1u);

    d3d12_ptr<ID3D12Resource> upload_resource;
    CD3DX12_HEAP_PROPERTIES upload_heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC upload_desc = CD3DX12_RESOURCE_DESC::Buffer(height * width_pitch);
    throw_if_failed(device->CreateCommittedResource(
        &upload_heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &upload_desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&upload_resource)));

    void* mapped = NULL;
    D3D12_RANGE range = {0, height * width_pitch};
    upload_resource->Map(0, &range, &mapped);
    for (std::size_t i = 0; i < height; ++i)
    {
        memcpy(
            static_cast<std::uint8_t*>(mapped) + i * width_pitch,
            data + i * width * 4,
            width * 4);
    }
    upload_resource->Unmap(0, &range);

    // Copy data to default buffer.
    D3D12_TEXTURE_COPY_LOCATION source_location = {};
    source_location.pResource = upload_resource.Get();
    source_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    source_location.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    source_location.PlacedFootprint.Footprint.Width = static_cast<UINT>(width);
    source_location.PlacedFootprint.Footprint.Height = static_cast<UINT>(height);
    source_location.PlacedFootprint.Footprint.Depth = 1;
    source_location.PlacedFootprint.Footprint.RowPitch = width_pitch;

    D3D12_TEXTURE_COPY_LOCATION target_location = {};
    target_location.pResource = m_resource.Get();
    target_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    target_location.SubresourceIndex = 0;

    command_list->CopyTextureRegion(&target_location, 0, 0, 0, &source_location, NULL);

    CD3DX12_RESOURCE_BARRIER transition = CD3DX12_RESOURCE_BARRIER::Transition(
        m_resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_GENERIC_READ);
    command_list->ResourceBarrier(1, &transition);

    d3d12_context::resource()->push_temporary_resource(upload_resource);

    // Create SRV.
    auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srv_offset = srv_heap->allocate(1);
    device->CreateShaderResourceView(m_resource.Get(), nullptr, srv_heap->cpu_handle(m_srv_offset));
}

d3d12_texture::~d3d12_texture()
{
    if (m_srv_offset != INVALID_DESCRIPTOR_INDEX)
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        heap->deallocate(m_srv_offset);
    }
}

d3d12_shader_resource_proxy d3d12_texture::shader_resource()
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return d3d12_shader_resource_proxy(heap->cpu_handle(m_srv_offset));
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
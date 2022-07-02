#include "d3d12_resource.hpp"
#include "d3d12_context.hpp"
#include "d3d12_image_loader.hpp"
#include "d3d12_pipeline.hpp"

namespace ash::graphics::d3d12
{
D3D12_CPU_DESCRIPTOR_HANDLE d3d12_resource::rtv() const
{
    throw d3d12_exception("This resource is not a render target.");
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_resource::dsv() const
{
    throw d3d12_exception("This resource is not a depth stencil buffer.");
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_resource::srv() const
{
    throw d3d12_exception("This resource is not a shader resource.");
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_resource::uav() const
{
    throw d3d12_exception("This resource is not a unordered access resource.");
}

void d3d12_resource::upload(const void* data, std::size_t size, std::size_t offset)
{
    throw d3d12_exception("This resource cannot be uploaded.");
}

resource_format d3d12_image::format() const noexcept
{
    return d3d12_utility::convert_format(m_resource->GetDesc().Format);
}

resource_extent d3d12_image::extent() const noexcept
{
    auto desc = m_resource->GetDesc();
    return resource_extent{
        static_cast<std::uint32_t>(desc.Width),
        static_cast<std::uint32_t>(desc.Height)};
}

d3d12_back_buffer::d3d12_back_buffer(d3d12_ptr<D3D12Resource> resource)
{
    m_resource = resource;
    m_resource_state = D3D12_RESOURCE_STATE_PRESENT;

    auto device = d3d12_context::device();

    // Create RTV.
    auto rtv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_rtv_offset = rtv_heap->allocate(1);
    device->CreateRenderTargetView(m_resource.Get(), nullptr, rtv_heap->cpu_handle(m_rtv_offset));
}

d3d12_back_buffer::d3d12_back_buffer(d3d12_back_buffer&& other)
{
    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;
    m_rtv_offset = other.m_rtv_offset;

    other.m_resource = nullptr;
    other.m_rtv_offset = INVALID_DESCRIPTOR_INDEX;
}

d3d12_back_buffer::~d3d12_back_buffer()
{
    if (m_rtv_offset != INVALID_DESCRIPTOR_INDEX)
    {
        d3d12_context::frame_buffer().notify_destroy(this);

        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        heap->deallocate(m_rtv_offset);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_back_buffer::rtv() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return heap->cpu_handle(m_rtv_offset);
}

d3d12_back_buffer& d3d12_back_buffer::operator=(d3d12_back_buffer&& other)
{
    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;
    m_rtv_offset = other.m_rtv_offset;

    other.m_resource = nullptr;
    other.m_rtv_offset = INVALID_DESCRIPTOR_INDEX;

    return *this;
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
    sample_level.Format = d3d12_utility::convert_format(format);
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = static_cast<UINT>(samples);
    throw_if_failed(device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    CD3DX12_RESOURCE_DESC render_target_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        d3d12_utility::convert_format(format),
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,
        1,
        sample_level.SampleCount,
        sample_level.NumQualityLevels - 1,
        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = d3d12_utility::convert_format(format);
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
    m_resource_state = D3D12_RESOURCE_STATE_RENDER_TARGET;

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

d3d12_render_target::d3d12_render_target(d3d12_render_target&& other)
    : m_rtv_offset(other.m_rtv_offset),
      m_srv_offset(other.m_srv_offset)
{
    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;

    other.m_resource = nullptr;
    other.m_rtv_offset = INVALID_DESCRIPTOR_INDEX;
    other.m_srv_offset = INVALID_DESCRIPTOR_INDEX;
}

d3d12_render_target::~d3d12_render_target()
{
    if (m_resource != nullptr)
    {
        d3d12_context::frame_buffer().notify_destroy(this);
        d3d12_context::resource()->delay_delete(m_resource);
    }

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

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_render_target::rtv() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return heap->cpu_handle(m_rtv_offset);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_render_target::srv() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return heap->cpu_handle(m_srv_offset);
}

d3d12_render_target& d3d12_render_target::operator=(d3d12_render_target&& other)
{
    if (m_resource != nullptr)
    {
        d3d12_context::frame_buffer().notify_destroy(this);
        d3d12_context::resource()->delay_delete(m_resource);
    }
    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;

    m_rtv_offset = other.m_rtv_offset;
    m_srv_offset = other.m_srv_offset;

    other.m_resource = nullptr;
    other.m_rtv_offset = INVALID_DESCRIPTOR_INDEX;
    other.m_srv_offset = INVALID_DESCRIPTOR_INDEX;

    return *this;
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
    sample_level.Format = d3d12_utility::convert_format(format);
    sample_level.NumQualityLevels = 0;
    sample_level.SampleCount = static_cast<UINT>(samples);
    throw_if_failed(device->CheckFeatureSupport(
        D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
        &sample_level,
        sizeof(sample_level)));

    CD3DX12_RESOURCE_DESC depth_stencil_desc = CD3DX12_RESOURCE_DESC::Tex2D(
        d3d12_utility::convert_format(format),
        static_cast<UINT>(width),
        static_cast<UINT>(height),
        1,
        1,
        sample_level.SampleCount,
        sample_level.NumQualityLevels - 1,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL | D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE);

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = d3d12_utility::convert_format(format);
    clear.DepthStencil.Depth = 1.0f;
    clear.DepthStencil.Stencil = 0;

    throw_if_failed(device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &depth_stencil_desc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clear,
        IID_PPV_ARGS(&m_resource)));
    m_resource_state = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    m_dsv_offset = heap->allocate(1);
    device->CreateDepthStencilView(m_resource.Get(), nullptr, heap->cpu_handle(m_dsv_offset));
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(const depth_stencil_buffer_desc& desc)
    : d3d12_depth_stencil_buffer(desc.width, desc.height, desc.samples, desc.format)
{
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(d3d12_depth_stencil_buffer&& other)
    : m_dsv_offset(other.m_dsv_offset)
{
    if (m_resource != nullptr)
    {
        d3d12_context::frame_buffer().notify_destroy(this);
        d3d12_context::resource()->delay_delete(m_resource);
    }
    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;

    other.m_resource = nullptr;
    other.m_dsv_offset = INVALID_DESCRIPTOR_INDEX;
}

d3d12_depth_stencil_buffer::~d3d12_depth_stencil_buffer()
{
    if (m_resource != nullptr)
    {
        d3d12_context::frame_buffer().notify_destroy(this);
        d3d12_context::resource()->delay_delete(m_resource);
    }

    if (m_dsv_offset != INVALID_DESCRIPTOR_INDEX)
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        heap->deallocate(m_dsv_offset);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_depth_stencil_buffer::dsv() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    return heap->cpu_handle(m_dsv_offset);
}

d3d12_depth_stencil_buffer& d3d12_depth_stencil_buffer::operator=(
    d3d12_depth_stencil_buffer&& other)
{
    if (m_resource != nullptr)
    {
        d3d12_context::frame_buffer().notify_destroy(this);
        d3d12_context::resource()->delay_delete(m_resource);
    }

    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;

    m_dsv_offset = other.m_dsv_offset;

    other.m_resource = nullptr;
    other.m_dsv_offset = INVALID_DESCRIPTOR_INDEX;

    return *this;
}

d3d12_texture::d3d12_texture(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    resource_format format,
    D3D12GraphicsCommandList* command_list)
{
    auto [resource, temporary] =
        d3d12_image_loader::load(data, width, height, format, command_list);
    m_resource = resource;
    d3d12_context::resource()->delay_delete(temporary);

    // Create SRV.
    auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srv_offset = srv_heap->allocate(1);
    d3d12_context::device()->CreateShaderResourceView(
        m_resource.Get(),
        nullptr,
        srv_heap->cpu_handle(m_srv_offset));

    resource_state(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

d3d12_texture::d3d12_texture(const char* file, D3D12GraphicsCommandList* command_list)
{
    auto [resource, temporary] = d3d12_image_loader::load(file, command_list);
    m_resource = resource;
    d3d12_context::resource()->delay_delete(temporary);

    // Create SRV.
    auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_srv_offset = srv_heap->allocate(1);
    d3d12_context::device()->CreateShaderResourceView(
        m_resource.Get(),
        nullptr,
        srv_heap->cpu_handle(m_srv_offset));

    resource_state(D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

d3d12_texture::d3d12_texture(d3d12_texture&& other) : m_srv_offset(other.m_srv_offset)
{
    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;

    other.m_resource = nullptr;
    other.m_srv_offset = INVALID_DESCRIPTOR_INDEX;
}

d3d12_texture::~d3d12_texture()
{
    if (m_resource != nullptr)
        d3d12_context::resource()->delay_delete(m_resource);

    if (m_srv_offset != INVALID_DESCRIPTOR_INDEX)
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        heap->deallocate(m_srv_offset);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_texture::srv() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return heap->cpu_handle(m_srv_offset);
}

d3d12_texture& d3d12_texture::operator=(d3d12_texture&& other)
{
    if (m_resource != nullptr)
        d3d12_context::resource()->delay_delete(m_resource);

    m_resource = other.m_resource;
    m_resource_state = other.m_resource_state;

    m_srv_offset = other.m_srv_offset;

    other.m_resource = nullptr;
    other.m_srv_offset = INVALID_DESCRIPTOR_INDEX;

    return *this;
}

d3d12_default_buffer::d3d12_default_buffer(
    const void* data,
    std::size_t size,
    D3D12GraphicsCommandList* command_list,
    D3D12_RESOURCE_FLAGS flags)
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
    CD3DX12_RESOURCE_DESC default_desc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);

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

    d3d12_context::resource()->delay_delete(upload_resource);
}

d3d12_upload_buffer::d3d12_upload_buffer(std::size_t size, D3D12_RESOURCE_FLAGS flags)
    : m_mapped(nullptr)
{
    CD3DX12_HEAP_PROPERTIES heap_properties(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(size, flags);

    throw_if_failed(d3d12_context::device()->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_resource)));
    resource_state(D3D12_RESOURCE_STATE_GENERIC_READ);

    throw_if_failed(m_resource->Map(0, nullptr, &m_mapped));
}

d3d12_upload_buffer::~d3d12_upload_buffer()
{
    if (m_mapped)
        m_resource->Unmap(0, nullptr);

    if (m_resource)
        d3d12_context::resource()->delay_delete(m_resource);
}

void d3d12_upload_buffer::upload(const void* data, std::size_t size, std::size_t offset)
{
    void* target = static_cast<std::uint8_t*>(m_mapped) + offset;
    std::memcpy(target, data, size);
}

void d3d12_upload_buffer::copy(std::size_t begin, std::size_t size, std::size_t target)
{
    void* mapped;
    throw_if_failed(m_resource->Map(0, nullptr, &mapped));

    void* source_ptr = static_cast<std::uint8_t*>(mapped) + begin;
    void* target_ptr = static_cast<std::uint8_t*>(mapped) + target;

    std::memcpy(target_ptr, source_ptr, size);

    m_resource->Unmap(0, nullptr);
}

d3d12_vertex_buffer_default::d3d12_vertex_buffer_default(
    const vertex_buffer_desc& desc,
    D3D12GraphicsCommandList* command_list)
{
    D3D12_RESOURCE_FLAGS flags = (desc.flags & VERTEX_BUFFER_FLAG_COMPUTE_OUT)
                                     ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                                     : D3D12_RESOURCE_FLAG_NONE;

    m_buffer = std::make_unique<d3d12_default_buffer>(
        desc.vertices,
        desc.vertex_size * desc.vertex_count,
        command_list,
        flags);
    m_view.BufferLocation = m_buffer->handle()->GetGPUVirtualAddress();
    m_view.SizeInBytes = static_cast<UINT>(desc.vertex_size * desc.vertex_count);
    m_view.StrideInBytes = static_cast<UINT>(desc.vertex_size);

    if (desc.flags & VERTEX_BUFFER_FLAG_COMPUTE_IN)
    {
        auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_srv_offset = srv_heap->allocate(1);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Buffer.FirstElement = 0;
        srv_desc.Buffer.NumElements = static_cast<UINT>(desc.vertex_count);
        srv_desc.Buffer.StructureByteStride = static_cast<UINT>(desc.vertex_size);
        srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        d3d12_context::device()->CreateShaderResourceView(
            m_buffer->handle(),
            &srv_desc,
            srv_heap->cpu_handle(m_srv_offset));
    }

    if (desc.flags & VERTEX_BUFFER_FLAG_COMPUTE_OUT)
    {
        auto uav_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_uav_offset = uav_heap->allocate(1);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.Buffer.FirstElement = 0;
        uav_desc.Buffer.NumElements = static_cast<UINT>(desc.vertex_count);
        uav_desc.Buffer.StructureByteStride = static_cast<UINT>(desc.vertex_size);
        uav_desc.Buffer.CounterOffsetInBytes = 0;
        uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        d3d12_context::device()->CreateUnorderedAccessView(
            m_buffer->handle(),
            nullptr,
            &uav_desc,
            uav_heap->cpu_handle(m_uav_offset));
    }
}

const D3D12_VERTEX_BUFFER_VIEW& d3d12_vertex_buffer_default::view() const noexcept
{
    return m_view;
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_vertex_buffer_default::srv() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return heap->cpu_handle(m_srv_offset);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_vertex_buffer_default::uav() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return heap->cpu_handle(m_uav_offset);
}

d3d12_vertex_buffer_dynamic::d3d12_vertex_buffer_dynamic(const vertex_buffer_desc& desc)
    : m_frame_resource(desc.frame_resource),
      m_current_index(0),
      m_last_sync_frame(0)
{
    m_size = desc.vertex_size * desc.vertex_count;
    D3D12_RESOURCE_FLAGS flags = (desc.flags & VERTEX_BUFFER_FLAG_COMPUTE_OUT)
                                     ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                                     : D3D12_RESOURCE_FLAG_NONE;

    std::size_t copy_count = desc.frame_resource ? d3d12_frame_counter::frame_resource_count() : 1;
    m_buffer = std::make_unique<d3d12_upload_buffer>(m_size * copy_count, flags);

    D3D12_VERTEX_BUFFER_VIEW view = {};
    view.BufferLocation = m_buffer->handle()->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(m_size);
    view.StrideInBytes = static_cast<UINT>(desc.vertex_size);

    for (std::size_t i = 0; i < copy_count; ++i)
    {
        m_views.push_back(view);
        view.BufferLocation += m_size;
    }

    if (desc.flags & VERTEX_BUFFER_FLAG_COMPUTE_IN)
    {
        auto srv_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_srv_offset = srv_heap->allocate(copy_count);

        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
        srv_desc.Format = DXGI_FORMAT_UNKNOWN;
        srv_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
        srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srv_desc.Buffer.NumElements = static_cast<UINT>(desc.vertex_count);
        srv_desc.Buffer.StructureByteStride = static_cast<UINT>(desc.vertex_size);
        srv_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

        for (std::size_t i = 0; i < copy_count; ++i)
        {
            d3d12_context::device()->CreateShaderResourceView(
                m_buffer->handle(),
                &srv_desc,
                srv_heap->cpu_handle(m_srv_offset + i));

            srv_desc.Buffer.FirstElement += static_cast<UINT>(desc.vertex_count);
        }
    }

    if (desc.flags & VERTEX_BUFFER_FLAG_COMPUTE_OUT)
    {
        auto uav_heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        m_uav_offset = uav_heap->allocate(copy_count);

        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {};
        uav_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
        uav_desc.Format = DXGI_FORMAT_UNKNOWN;
        uav_desc.Buffer.NumElements = static_cast<UINT>(desc.vertex_count);
        uav_desc.Buffer.StructureByteStride = static_cast<UINT>(desc.vertex_size);
        uav_desc.Buffer.CounterOffsetInBytes = 0;
        uav_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

        for (std::size_t i = 0; i < copy_count; ++i)
        {
            d3d12_context::device()->CreateUnorderedAccessView(
                m_buffer->handle(),
                nullptr,
                &uav_desc,
                uav_heap->cpu_handle(m_uav_offset));

            uav_desc.Buffer.FirstElement += static_cast<UINT>(desc.vertex_count);
        }
    }

    if (desc.vertices != nullptr && m_size != 0)
        m_buffer->upload(desc.vertices, m_size, 0);
}

void d3d12_vertex_buffer_dynamic::upload(const void* data, std::size_t size, std::size_t offset)
{
    if (m_frame_resource)
    {
        std::size_t current_frame = d3d12_frame_counter::frame_counter();
        if (m_last_sync_frame != current_frame)
        {
            std::size_t frame_resource_count = d3d12_frame_counter::frame_resource_count();
            std::size_t next_index = (m_current_index + 1) % frame_resource_count;

            std::size_t begin = m_current_index * m_size;
            std::size_t target = next_index * m_size;
            m_buffer->copy(begin, m_size, target);

            m_last_sync_frame = current_frame;
            m_current_index = next_index;
        }

        m_buffer->upload(data, size, offset + m_size * m_current_index);
    }
    else
    {
        m_buffer->upload(data, size, offset);
    }
}

const D3D12_VERTEX_BUFFER_VIEW& d3d12_vertex_buffer_dynamic::view() const noexcept
{
    return m_views[m_current_index];
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_vertex_buffer_dynamic::srv() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return heap->cpu_handle(m_srv_offset + m_current_index);
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_vertex_buffer_dynamic::uav() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    return heap->cpu_handle(m_uav_offset + m_current_index);
}

d3d12_index_buffer::d3d12_index_buffer(std::size_t index_count) : m_index_count(index_count)
{
}

d3d12_index_buffer_default::d3d12_index_buffer_default(
    const index_buffer_desc& desc,
    D3D12GraphicsCommandList* command_list)
    : d3d12_index_buffer(desc.index_count)
{
    m_buffer = std::make_unique<d3d12_default_buffer>(
        desc.indices,
        desc.index_size * desc.index_count,
        command_list);

    m_view.BufferLocation = m_buffer->handle()->GetGPUVirtualAddress();
    m_view.SizeInBytes = static_cast<UINT>(desc.index_size * desc.index_count);

    if (desc.index_size == 1)
        m_view.Format = DXGI_FORMAT_R8_UINT;
    else if (desc.index_size == 2)
        m_view.Format = DXGI_FORMAT_R16_UINT;
    else if (desc.index_size == 4)
        m_view.Format = DXGI_FORMAT_R32_UINT;
    else
        throw std::out_of_range("Invalid index size.");
}

const D3D12_INDEX_BUFFER_VIEW& d3d12_index_buffer_default::view() const noexcept
{
    return m_view;
}

d3d12_index_buffer_dynamic::d3d12_index_buffer_dynamic(const index_buffer_desc& desc)
    : d3d12_index_buffer(desc.index_count),
      m_frame_resource(desc.frame_resource),
      m_current_index(0),
      m_last_sync_frame(0)
{
    m_size = desc.index_size * desc.index_count;

    std::size_t copy_count = desc.frame_resource ? d3d12_frame_counter::frame_resource_count() : 1;
    m_buffer = std::make_unique<d3d12_upload_buffer>(m_size * copy_count);

    D3D12_INDEX_BUFFER_VIEW view = {};
    view.BufferLocation = m_buffer->handle()->GetGPUVirtualAddress();
    view.SizeInBytes = static_cast<UINT>(m_size);

    if (desc.index_size == 1)
        view.Format = DXGI_FORMAT_R8_UINT;
    else if (desc.index_size == 2)
        view.Format = DXGI_FORMAT_R16_UINT;
    else if (desc.index_size == 4)
        view.Format = DXGI_FORMAT_R32_UINT;
    else
        throw std::out_of_range("Invalid index size.");

    for (std::size_t i = 0; i < copy_count; ++i)
    {
        m_views.push_back(view);
        view.BufferLocation += m_size;
    }

    if (desc.indices != nullptr && m_size != 0)
        m_buffer->upload(desc.indices, m_size, 0);
}

void d3d12_index_buffer_dynamic::upload(const void* data, std::size_t size, std::size_t offset)
{
    if (m_frame_resource)
    {
        std::size_t current_frame = d3d12_frame_counter::frame_counter();
        if (m_last_sync_frame != current_frame)
        {
            std::size_t frame_resource_count = d3d12_frame_counter::frame_resource_count();
            std::size_t next_index = (m_current_index + 1) % frame_resource_count;

            std::size_t begin = (m_current_index % frame_resource_count) * m_size;
            std::size_t target = next_index * m_size;
            m_buffer->copy(begin, m_size, target);

            m_last_sync_frame = current_frame;
            m_current_index = next_index;
        }

        m_buffer->upload(data, size, offset + m_size * m_current_index);
    }
    else
    {
        m_buffer->upload(data, size, offset);
    }
}

const D3D12_INDEX_BUFFER_VIEW& d3d12_index_buffer_dynamic::view() const noexcept
{
    return m_views[m_current_index];
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

void d3d12_resource_manager::delay_delete(d3d12_ptr<D3D12Resource> resource)
{
    m_temporary.get().push_back(resource);
}

void d3d12_resource_manager::switch_frame_resources()
{
    m_temporary.get().clear();
}
} // namespace ash::graphics::d3d12
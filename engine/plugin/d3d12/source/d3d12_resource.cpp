#include "d3d12_resource.hpp"
#include "DDSTextureLoader12.h"
#include "d3d12_context.hpp"
#include <fstream>

namespace ash::graphics::d3d12
{
d3d12_resource::d3d12_resource() noexcept : d3d12_resource(nullptr, D3D12_RESOURCE_STATE_COMMON)
{
}

d3d12_resource::d3d12_resource(
    d3d12_ptr<D3D12Resource> resource,
    D3D12_RESOURCE_STATES state) noexcept
    : m_resource(resource),
      m_state(state)
{
}

void d3d12_resource::transition_state(
    D3D12_RESOURCE_STATES state,
    D3D12GraphicsCommandList* command_list)
{
    CD3DX12_RESOURCE_BARRIER transition =
        CD3DX12_RESOURCE_BARRIER::Transition(m_resource.Get(), m_state, state);
    command_list->ResourceBarrier(1, &transition);
    m_state = state;
}

std::size_t d3d12_resource::size() const
{
    auto desc = m_resource->GetDesc();
    return static_cast<std::size_t>(desc.Height * desc.Width);
}

d3d12_back_buffer::d3d12_back_buffer() noexcept : m_descriptor_offset(INVALID_DESCRIPTOR_INDEX)
{
}

d3d12_back_buffer::d3d12_back_buffer(
    d3d12_ptr<D3D12Resource> resource,
    D3D12_RESOURCE_STATES state) noexcept
    : d3d12_resource(resource, state)
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    m_descriptor_offset = heap->allocate(1);

    d3d12_context::device()->CreateRenderTargetView(
        m_resource.Get(),
        nullptr,
        heap->cpu_handle(m_descriptor_offset));
}

d3d12_back_buffer::d3d12_back_buffer(d3d12_back_buffer&& other) noexcept
    : d3d12_resource(std::move(other)),
      m_descriptor_offset(other.m_descriptor_offset)
{
    other.m_descriptor_offset = INVALID_DESCRIPTOR_INDEX;
}

d3d12_back_buffer::~d3d12_back_buffer()
{
    if (m_descriptor_offset != INVALID_DESCRIPTOR_INDEX)
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        heap->deallocate(m_descriptor_offset);
    }
}

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_back_buffer::cpu_handle() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    return heap->cpu_handle(m_descriptor_offset);
}

d3d12_back_buffer& d3d12_back_buffer::operator=(d3d12_back_buffer&& other) noexcept
{
    if (this != &other)
    {
        d3d12_resource::operator=(std::move(other));
        m_descriptor_offset = other.m_descriptor_offset;
        other.m_descriptor_offset = INVALID_DESCRIPTOR_INDEX;
    }

    return *this;
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer() noexcept
    : m_descriptor_offset(INVALID_DESCRIPTOR_INDEX)
{
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(
    const D3D12_RESOURCE_DESC& desc,
    const D3D12_HEAP_PROPERTIES& heap_properties,
    const D3D12_CLEAR_VALUE& clear)
{
    auto device = d3d12_context::device();

    throw_if_failed(device->CreateCommittedResource(
        &heap_properties,
        D3D12_HEAP_FLAG_NONE,
        &desc,
        D3D12_RESOURCE_STATE_COMMON,
        &clear,
        IID_PPV_ARGS(&m_resource)));

    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

    m_descriptor_offset = heap->allocate(1);
    device->CreateDepthStencilView(
        m_resource.Get(),
        nullptr,
        heap->cpu_handle(m_descriptor_offset));
}

d3d12_depth_stencil_buffer::d3d12_depth_stencil_buffer(d3d12_depth_stencil_buffer&& other) noexcept
    : d3d12_resource(std::move(other)),
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

D3D12_CPU_DESCRIPTOR_HANDLE d3d12_depth_stencil_buffer::cpu_handle() const
{
    auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    return heap->cpu_handle(m_descriptor_offset);
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
    : d3d12_resource(nullptr, D3D12_RESOURCE_STATE_COPY_DEST)
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

    transition_state(D3D12_RESOURCE_STATE_GENERIC_READ, command_list);

    d3d12_context::resource()->push_temporary_resource(upload_resource);
}

d3d12_upload_buffer::d3d12_upload_buffer() noexcept : m_mapped(nullptr)
{
}

d3d12_upload_buffer::d3d12_upload_buffer(std::size_t size)
    : d3d12_resource(nullptr, D3D12_RESOURCE_STATE_GENERIC_READ)
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

    throw_if_failed(m_resource->Map(0, nullptr, &m_mapped));
}

d3d12_upload_buffer::d3d12_upload_buffer(d3d12_upload_buffer&& other) noexcept
    : d3d12_resource(std::move(other)),
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

d3d12_vertex_buffer::d3d12_vertex_buffer(
    const void* data,
    std::size_t vertex_size,
    std::size_t vertex_count,
    D3D12GraphicsCommandList* command_list)
    : d3d12_default_buffer(data, vertex_size * vertex_count, command_list)
{
    m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
    m_view.SizeInBytes = static_cast<UINT>(vertex_size * vertex_count);
    m_view.StrideInBytes = static_cast<UINT>(vertex_size);
}

d3d12_index_buffer::d3d12_index_buffer(
    const void* data,
    std::size_t index_size,
    std::size_t index_count,
    D3D12GraphicsCommandList* command_list)
    : d3d12_default_buffer(data, index_size * index_count, command_list),
      m_index_count(index_count)
{
    m_view.BufferLocation = m_resource->GetGPUVirtualAddress();
    m_view.SizeInBytes = static_cast<UINT>(index_size * index_count);

    if (index_size == 2)
        m_view.Format = DXGI_FORMAT_R16_UINT;
    else if (index_size == 4)
        m_view.Format = DXGI_FORMAT_R32_UINT;
    else
        throw std::out_of_range("Invalid index size.");
}

d3d12_texture::d3d12_texture(const char* file, D3D12GraphicsCommandList* command_list)
    : d3d12_resource(nullptr, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
{
    std::ifstream fin(file, std::ios::in | std::ios::binary);
    if (!fin)
        return;

    std::vector<uint8_t> dds_data(fin.seekg(0, std::ios::end).tellg());
    fin.seekg(0, std::ios::beg).read((char*)dds_data.data(), dds_data.size());
    fin.close();

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    throw_if_failed(DirectX::LoadDDSTextureFromMemory(
        d3d12_context::device(),
        dds_data.data(),
        dds_data.size(),
        m_resource.GetAddressOf(),
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
        IID_PPV_ARGS(upload_resource.GetAddressOf())));

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

    d3d12_context::resource()->push_temporary_resource(upload_resource);
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
#pragma once

#include "d3d12_common.hpp"
#include "d3d12_context.hpp"
#include "d3d12_descriptor_heap.hpp"

namespace ash::graphics::d3d12
{
class d3d12_vertex_buffer_proxy
{
public:
    d3d12_vertex_buffer_proxy(const D3D12_VERTEX_BUFFER_VIEW& view) : m_view(view) {}

    const D3D12_VERTEX_BUFFER_VIEW& view() const noexcept { return m_view; }

private:
    const D3D12_VERTEX_BUFFER_VIEW& m_view;
};

class d3d12_index_buffer_proxy
{
public:
    d3d12_index_buffer_proxy(const D3D12_INDEX_BUFFER_VIEW& view, std::size_t index_count)
        : m_view(view),
          m_index_count(index_count)
    {
    }

    inline const D3D12_INDEX_BUFFER_VIEW& view() const noexcept { return m_view; }
    inline std::size_t index_count() const noexcept { return m_index_count; }

private:
    const D3D12_INDEX_BUFFER_VIEW& m_view;
    std::size_t m_index_count;
};

class d3d12_resource : public resource_interface
{
public:
    virtual ~d3d12_resource() = default;

    virtual D3D12_CPU_DESCRIPTOR_HANDLE rtv() const
    {
        throw d3d12_exception("The resource is not a render target");
    }

    virtual D3D12_CPU_DESCRIPTOR_HANDLE dsv() const
    {
        throw d3d12_exception("The resource is not a depth stencil buffer");
    }

    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const
    {
        throw d3d12_exception("The resource is not a shader resource");
    }

    virtual D3D12_CPU_DESCRIPTOR_HANDLE uav() const
    {
        throw d3d12_exception("The resource is not a unordered access resource");
    }

    virtual d3d12_vertex_buffer_proxy vertex_buffer();
    virtual d3d12_index_buffer_proxy index_buffer();

    virtual D3D12Resource* handle() const noexcept = 0;

    virtual resource_format format() const noexcept override { return resource_format::UNDEFINED; }
    virtual resource_extent extent() const noexcept override { return {0, 0}; }
    virtual std::size_t size() const noexcept override { return 0; }

    inline void resource_state(D3D12_RESOURCE_STATES state) noexcept { m_resource_state = state; }
    inline D3D12_RESOURCE_STATES resource_state() const noexcept { return m_resource_state; }

private:
    D3D12_RESOURCE_STATES m_resource_state;
};

class d3d12_render_target : public d3d12_resource
{
public:
    d3d12_render_target(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t samples,
        resource_format format);
    d3d12_render_target(const render_target_desc& desc);
    d3d12_render_target(d3d12_ptr<D3D12Resource> resource);
    d3d12_render_target(d3d12_render_target&& other);
    virtual ~d3d12_render_target();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE rtv() const override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override;

    virtual D3D12Resource* handle() const noexcept override { return m_resource.Get(); }

    virtual resource_format format() const noexcept override;
    virtual resource_extent extent() const noexcept override;

    d3d12_render_target& operator=(d3d12_render_target&& other);

private:
    d3d12_ptr<D3D12Resource> m_resource;
    std::size_t m_rtv_offset;
    std::size_t m_srv_offset;
};

class d3d12_depth_stencil_buffer : public d3d12_resource
{
public:
    d3d12_depth_stencil_buffer(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t samples,
        resource_format format);
    d3d12_depth_stencil_buffer(const depth_stencil_buffer_desc& desc);
    d3d12_depth_stencil_buffer(d3d12_depth_stencil_buffer&& other);
    virtual ~d3d12_depth_stencil_buffer();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE dsv() const override;
    virtual D3D12Resource* handle() const noexcept override { return m_resource.Get(); }
    virtual resource_format format() const noexcept override;
    virtual resource_extent extent() const noexcept override;

    d3d12_depth_stencil_buffer& operator=(d3d12_depth_stencil_buffer&& other);

private:
    d3d12_ptr<D3D12Resource> m_resource;
    std::size_t m_dsv_offset;
};

class d3d12_texture : public d3d12_resource
{
public:
    d3d12_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        D3D12GraphicsCommandList* command_list);
    d3d12_texture(const char* file, D3D12GraphicsCommandList* command_list);

    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override;
    virtual D3D12Resource* handle() const noexcept override { return m_resource.Get(); }

private:
    d3d12_ptr<D3D12Resource> m_resource;
    std::size_t m_srv_offset;
};

class d3d12_default_buffer : public d3d12_resource
{
public:
    d3d12_default_buffer(
        const void* data,
        std::size_t size,
        D3D12GraphicsCommandList* command_list,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    virtual D3D12Resource* handle() const noexcept override { return m_resource.Get(); }

private:
    d3d12_ptr<D3D12Resource> m_resource;
};

class d3d12_upload_buffer : public d3d12_resource
{
public:
    d3d12_upload_buffer(
        const void* data,
        std::size_t size,
        D3D12GraphicsCommandList* command_list = nullptr,
        D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);

    virtual void upload(const void* data, std::size_t size, std::size_t offset) override;

    virtual D3D12Resource* handle() const noexcept override { return m_resource.Get(); }

private:
    d3d12_ptr<D3D12Resource> m_resource;
};

template <typename Impl>
class d3d12_vertex_buffer : public Impl
{
public:
    d3d12_vertex_buffer() = default;
    d3d12_vertex_buffer(const vertex_buffer_desc& desc, D3D12GraphicsCommandList* command_list)
        : Impl(
              desc.vertices,
              desc.vertex_size * desc.vertex_count,
              command_list,
              (desc.flags & VERTEX_BUFFER_FLAG_SKIN_OUT)
                  ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
                  : D3D12_RESOURCE_FLAG_NONE),
          m_uav_offset(INVALID_DESCRIPTOR_INDEX)
    {
        m_view.BufferLocation = Impl::handle()->GetGPUVirtualAddress();
        m_view.SizeInBytes = static_cast<UINT>(desc.vertex_size * desc.vertex_count);
        m_view.StrideInBytes = static_cast<UINT>(desc.vertex_size);

        if (desc.flags & VERTEX_BUFFER_FLAG_SKIN_IN)
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
                Impl::handle(),
                &srv_desc,
                srv_heap->cpu_handle(m_srv_offset));
        }

        if (desc.flags & VERTEX_BUFFER_FLAG_SKIN_OUT)
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
                Impl::handle(),
                nullptr,
                &uav_desc,
                uav_heap->cpu_handle(m_uav_offset));
        }
    }

    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return heap->cpu_handle(m_srv_offset);
    }

    virtual D3D12_CPU_DESCRIPTOR_HANDLE uav() const override
    {
        auto heap = d3d12_context::resource()->heap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        return heap->cpu_handle(m_uav_offset);
    }

    virtual d3d12_vertex_buffer_proxy vertex_buffer() noexcept override
    {
        return d3d12_vertex_buffer_proxy(m_view);
    }

private:
    D3D12_VERTEX_BUFFER_VIEW m_view;
    std::size_t m_srv_offset;
    std::size_t m_uav_offset;
};

template <typename Impl>
class d3d12_index_buffer : public Impl
{
public:
    d3d12_index_buffer(const index_buffer_desc& desc, D3D12GraphicsCommandList* command_list)
        : Impl(desc.indices, desc.index_size * desc.index_count, command_list)
    {
        m_view.BufferLocation = Impl::handle()->GetGPUVirtualAddress();
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

    virtual d3d12_index_buffer_proxy index_buffer() noexcept override
    {
        return d3d12_index_buffer_proxy(m_view, m_index_count);
    }

private:
    D3D12_INDEX_BUFFER_VIEW m_view;
    std::size_t m_index_count;
};

class d3d12_resource_manager
{
public:
    using heap_list =
        std::array<std::unique_ptr<d3d12_descriptor_heap>, D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES>;
    using temporary_list = std::vector<d3d12_ptr<D3D12Resource>>;

public:
    d3d12_resource_manager();

    d3d12_descriptor_heap* heap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        return m_heaps[type].get();
    }

    d3d12_descriptor_heap* visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        return m_visible_heaps[type].get();
    }

    void push_temporary_resource(d3d12_ptr<D3D12Resource> resource);
    void switch_frame_resources();

private:
    heap_list m_heaps;
    heap_list m_visible_heaps;

    d3d12_frame_resource<temporary_list> m_temporary;
};
} // namespace ash::graphics::d3d12
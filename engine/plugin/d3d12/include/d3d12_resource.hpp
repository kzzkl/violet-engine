#pragma once

#include "d3d12_common.hpp"
#include "d3d12_frame_resource.hpp"
#include <array>
#include <deque>

namespace ash::graphics::d3d12
{
class d3d12_render_target;
class d3d12_render_target_proxy
{
public:
    explicit d3d12_render_target_proxy(D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle)
        : m_rtv_handle(rtv_handle)
    {
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle() const noexcept { return m_rtv_handle; }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_rtv_handle;
};

class d3d12_depth_stencil_buffer;
class d3d12_depth_stencil_buffer_proxy
{
public:
    explicit d3d12_depth_stencil_buffer_proxy(D3D12_CPU_DESCRIPTOR_HANDLE dsv_handle)
        : m_dsv_handle(dsv_handle)
    {
    }

    inline D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle() const noexcept { return m_dsv_handle; }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_dsv_handle;
};

class d3d12_shader_resource_proxy
{
public:
    explicit d3d12_shader_resource_proxy(D3D12_CPU_DESCRIPTOR_HANDLE srv_handle)
        : m_srv_handle(srv_handle)
    {
    }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle() const noexcept { return m_srv_handle; }

private:
    D3D12_CPU_DESCRIPTOR_HANDLE m_srv_handle;
};

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

    virtual d3d12_render_target_proxy render_target();
    virtual d3d12_depth_stencil_buffer_proxy depth_stencil_buffer();
    virtual d3d12_shader_resource_proxy shader_resource();
    virtual d3d12_vertex_buffer_proxy vertex_buffer();
    virtual d3d12_index_buffer_proxy index_buffer();

    virtual D3D12Resource* resource() const noexcept = 0;

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

    virtual d3d12_render_target_proxy render_target() override;
    virtual d3d12_shader_resource_proxy shader_resource() override;

    virtual D3D12Resource* resource() const noexcept override { return m_resource.Get(); }

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

    virtual d3d12_depth_stencil_buffer_proxy depth_stencil_buffer();

    virtual D3D12Resource* resource() const noexcept override { return m_resource.Get(); }

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

    virtual d3d12_shader_resource_proxy shader_resource() override;

    virtual D3D12Resource* resource() const noexcept override { return m_resource.Get(); }

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
        D3D12GraphicsCommandList* command_list);

    virtual D3D12Resource* resource() const noexcept override { return m_resource.Get(); }

private:
    d3d12_ptr<D3D12Resource> m_resource;
};

class d3d12_upload_buffer : public d3d12_resource
{
public:
    d3d12_upload_buffer(
        const void* data,
        std::size_t size,
        D3D12GraphicsCommandList* command_list = nullptr);

    virtual void upload(const void* data, std::size_t size, std::size_t offset) override;

    virtual D3D12Resource* resource() const noexcept override { return m_resource.Get(); }

private:
    d3d12_ptr<D3D12Resource> m_resource;
};

template <typename Impl>
class d3d12_vertex_buffer : public Impl
{
public:
    d3d12_vertex_buffer() = default;
    d3d12_vertex_buffer(const vertex_buffer_desc& desc, D3D12GraphicsCommandList* command_list)
        : Impl(desc.vertices, desc.vertex_size * desc.vertex_count, command_list)
    {
        m_view.BufferLocation = Impl::resource()->GetGPUVirtualAddress();
        m_view.SizeInBytes = static_cast<UINT>(desc.vertex_size * desc.vertex_count);
        m_view.StrideInBytes = static_cast<UINT>(desc.vertex_size);
    }

    virtual d3d12_vertex_buffer_proxy vertex_buffer() noexcept override
    {
        return d3d12_vertex_buffer_proxy(m_view);
    }

private:
    D3D12_VERTEX_BUFFER_VIEW m_view;
};

template <typename Impl>
class d3d12_index_buffer : public Impl
{
public:
    d3d12_index_buffer(const index_buffer_desc& desc, D3D12GraphicsCommandList* command_list)
        : Impl(desc.indices, desc.index_size * desc.index_count, command_list)
    {
        m_view.BufferLocation = Impl::resource()->GetGPUVirtualAddress();
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

template <typename T>
class index_allocator
{
public:
    using value_type = T;

    struct index_range
    {
        value_type begin;
        value_type end;
    };

public:
    index_allocator() : m_next_index(0) {}

    value_type allocate(std::size_t size = 1)
    {
        for (auto iter = m_free.begin(); iter < m_free.end(); ++iter)
        {
            if (iter->end - iter->begin >= size)
            {
                value_type result = iter->begin;

                iter->begin += static_cast<value_type>(size);
                if (iter->begin == iter->end)
                    m_free.erase(iter);

                return result;
            }
        }

        value_type reuslt = m_next_index;
        m_next_index += static_cast<value_type>(size);

        return reuslt;
    }

    void deallocate(const value_type& begin, std::size_t size = 1)
    {
        value_type end = begin + size;
        for (index_range& free_range : m_free)
        {
            if (free_range.begin == end)
            {
                free_range.begin -= size;
                return;
            }
            else if (free_range.end == begin)
            {
                free_range.end += size;
                return;
            }
        }

        m_free.push_back(index_range{begin, end});
    }

private:
    std::deque<index_range> m_free;
    value_type m_next_index;
};

static constexpr std::size_t INVALID_DESCRIPTOR_INDEX = -1;

class d3d12_descriptor_heap
{
public:
    d3d12_descriptor_heap(
        D3D12_DESCRIPTOR_HEAP_TYPE type,
        std::size_t size,
        std::size_t increment_size,
        D3D12_DESCRIPTOR_HEAP_FLAGS flag = D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

    std::size_t allocate(std::size_t size = 1);
    void deallocate(std::size_t begin, std::size_t size = 1);

    inline D3D12DescriptorHeap* heap() const noexcept { return m_heap.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle(std::size_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle(std::size_t index);

    inline UINT increment_size() const noexcept { return m_increment_size; }

private:
    index_allocator<std::size_t> m_index_allocator;
    d3d12_ptr<D3D12DescriptorHeap> m_heap;

    UINT m_increment_size;
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
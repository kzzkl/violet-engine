#pragma once

#include "d3d12_common.hpp"
#include "d3d12_frame_resource.hpp"
#include <array>
#include <deque>

namespace ash::graphics::d3d12
{
class d3d12_resource : public resource
{
public:
    d3d12_resource();
    d3d12_resource(
        d3d12_ptr<D3D12Resource> resource,
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);
    d3d12_resource(const d3d12_resource&) = delete;
    d3d12_resource(d3d12_resource&&) = default;

    virtual ~d3d12_resource() = default;

    D3D12Resource* get_resource() const { return m_resource.Get(); }

    void transition_state(D3D12_RESOURCE_STATES state, D3D12GraphicsCommandList* command_list);

    std::size_t get_size() const;

    d3d12_resource& operator=(const d3d12_resource&) = delete;
    d3d12_resource& operator=(d3d12_resource&&) = default;

protected:
    d3d12_ptr<D3D12Resource> m_resource;
    D3D12_RESOURCE_STATES m_state;
};

class d3d12_back_buffer : public d3d12_resource
{
public:
    d3d12_back_buffer();
    d3d12_back_buffer(
        d3d12_ptr<D3D12Resource> resource,
        D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_COMMON);
    d3d12_back_buffer(const d3d12_back_buffer&) = delete;
    d3d12_back_buffer(d3d12_back_buffer&& other);

    virtual ~d3d12_back_buffer();

    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle() const;

    d3d12_back_buffer& operator=(const d3d12_back_buffer&) = delete;
    d3d12_back_buffer& operator=(d3d12_back_buffer&& other);

private:
    std::size_t m_descriptor_offset;
};

class d3d12_depth_stencil_buffer : public d3d12_resource
{
public:
    d3d12_depth_stencil_buffer();
    d3d12_depth_stencil_buffer(
        const D3D12_RESOURCE_DESC& desc,
        const D3D12_HEAP_PROPERTIES& heap_properties,
        const D3D12_CLEAR_VALUE& clear);
    d3d12_depth_stencil_buffer(const d3d12_depth_stencil_buffer&) = delete;
    d3d12_depth_stencil_buffer(d3d12_depth_stencil_buffer&& other);

    virtual ~d3d12_depth_stencil_buffer();

    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle() const;

    d3d12_depth_stencil_buffer& operator=(const d3d12_depth_stencil_buffer&) = delete;
    d3d12_depth_stencil_buffer& operator=(d3d12_depth_stencil_buffer&& other);

private:
    std::size_t m_descriptor_offset;
};

class d3d12_default_buffer : public d3d12_resource
{
public:
    d3d12_default_buffer() = default;
    d3d12_default_buffer(
        const void* data,
        std::size_t size,
        D3D12GraphicsCommandList* command_list);
};

class d3d12_upload_buffer : public d3d12_resource
{
public:
    d3d12_upload_buffer();
    d3d12_upload_buffer(std::size_t size);
    d3d12_upload_buffer(const d3d12_upload_buffer&) = delete;
    d3d12_upload_buffer(d3d12_upload_buffer&& other);

    virtual ~d3d12_upload_buffer();

    void upload(const void* data, std::size_t size);

    d3d12_upload_buffer& operator=(const d3d12_upload_buffer&) = delete;
    d3d12_upload_buffer& operator=(d3d12_upload_buffer&& other);

private:
    void* m_mapped;
};

class d3d12_vertex_buffer : public d3d12_default_buffer
{
public:
    d3d12_vertex_buffer(
        const void* data,
        std::size_t vertex_size,
        std::size_t vertex_count,
        D3D12GraphicsCommandList* command_list);

    const D3D12_VERTEX_BUFFER_VIEW& get_view() const { return m_view; }

private:
    D3D12_VERTEX_BUFFER_VIEW m_view;
};

class d3d12_index_buffer : public d3d12_default_buffer
{
public:
    d3d12_index_buffer(
        const void* data,
        std::size_t index_size,
        std::size_t index_count,
        D3D12GraphicsCommandList* command_list);

    const D3D12_INDEX_BUFFER_VIEW& get_view() const { return m_view; }

    std::size_t get_index_count() const { return m_index_count; }

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

    D3D12DescriptorHeap* get_heap() const { return m_heap.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_handle(std::size_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE get_gpu_handle(std::size_t index);

    UINT get_increment_size() const { return m_increment_size; }

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

    d3d12_descriptor_heap* get_heap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
    {
        return m_heaps[type].get();
    }

    d3d12_descriptor_heap* get_visible_heap(D3D12_DESCRIPTOR_HEAP_TYPE type) const
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
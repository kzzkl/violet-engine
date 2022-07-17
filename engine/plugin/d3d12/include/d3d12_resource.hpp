#pragma once

#include "d3d12_common.hpp"
#include "d3d12_context.hpp"
#include "d3d12_descriptor_heap.hpp"

namespace ash::graphics::d3d12
{
class d3d12_resource : public resource_interface
{
public:
    d3d12_resource();
    d3d12_resource(const d3d12_resource&) = delete;
    virtual ~d3d12_resource() = default;

    virtual D3D12_CPU_DESCRIPTOR_HANDLE rtv() const;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE dsv() const;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE uav() const;

    virtual D3D12Resource* handle() const noexcept = 0;

    virtual resource_format format() const noexcept override { return RESOURCE_FORMAT_UNDEFINED; }
    virtual resource_extent extent() const noexcept override { return {0, 0}; }
    virtual std::size_t size() const noexcept override { return 0; }

    virtual void* pointer() override;
    virtual void upload(const void* data, std::size_t size, std::size_t offset) override;

    inline void resource_state(D3D12_RESOURCE_STATES state) noexcept { m_resource_state = state; }
    inline D3D12_RESOURCE_STATES resource_state() const noexcept { return m_resource_state; }

    d3d12_resource& operator=(const d3d12_resource&) = delete;

protected:
    D3D12_RESOURCE_STATES m_resource_state;
};

class d3d12_image : public d3d12_resource
{
public:
    virtual resource_format format() const noexcept override;
    virtual resource_extent extent() const noexcept override;

    virtual D3D12Resource* handle() const noexcept override { return m_resource.Get(); }

protected:
    d3d12_ptr<D3D12Resource> m_resource;
};

class d3d12_back_buffer : public d3d12_image
{
public:
    d3d12_back_buffer(d3d12_ptr<D3D12Resource> resource);
    virtual ~d3d12_back_buffer();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE rtv() const override;

private:
    std::size_t m_rtv_offset;
};

class d3d12_render_target : public d3d12_image
{
public:
    d3d12_render_target(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t samples,
        resource_format format);
    d3d12_render_target(const render_target_desc& desc);
    virtual ~d3d12_render_target();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE rtv() const override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override;

private:
    std::size_t m_rtv_offset;
    std::size_t m_srv_offset;
};

class d3d12_depth_stencil_buffer : public d3d12_image
{
public:
    d3d12_depth_stencil_buffer(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t samples,
        resource_format format);
    d3d12_depth_stencil_buffer(const depth_stencil_buffer_desc& desc);
    virtual ~d3d12_depth_stencil_buffer();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE dsv() const override;

private:
    std::size_t m_dsv_offset;
};

class d3d12_texture : public d3d12_image
{
public:
    d3d12_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format,
        D3D12GraphicsCommandList* command_list);
    d3d12_texture(const char* file, D3D12GraphicsCommandList* command_list);
    virtual ~d3d12_texture();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override;

private:
    std::size_t m_srv_offset;
};

class d3d12_texture_cube : public d3d12_image
{
public:
    d3d12_texture_cube(
        const std::vector<std::string_view>& files,
        D3D12GraphicsCommandList* command_list);
    virtual ~d3d12_texture_cube();

    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override;

private:
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
    virtual std::size_t size() const noexcept override { return m_resource->GetDesc().Width; }

private:
    d3d12_ptr<D3D12Resource> m_resource;
};

class d3d12_upload_buffer : public d3d12_resource
{
public:
    d3d12_upload_buffer(std::size_t size, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE);
    virtual ~d3d12_upload_buffer();

    virtual void* pointer() override { return m_mapped; }
    virtual void upload(const void* data, std::size_t size, std::size_t offset) override;
    void copy(std::size_t begin, std::size_t size, std::size_t target);

    void* mapped_pointer() const noexcept { return m_mapped; }
    virtual D3D12Resource* handle() const noexcept override { return m_resource.Get(); }
    virtual std::size_t size() const noexcept override { return m_resource->GetDesc().Width; }

private:
    d3d12_ptr<D3D12Resource> m_resource;
    void* m_mapped;
};

class d3d12_vertex_buffer : public d3d12_resource
{
public:
    virtual ~d3d12_vertex_buffer() = default;
    virtual const D3D12_VERTEX_BUFFER_VIEW& view() const noexcept = 0;
};

class d3d12_vertex_buffer_default : public d3d12_vertex_buffer
{
public:
    d3d12_vertex_buffer_default(
        const vertex_buffer_desc& desc,
        D3D12GraphicsCommandList* command_list);
    virtual ~d3d12_vertex_buffer_default() = default;

    virtual const D3D12_VERTEX_BUFFER_VIEW& view() const noexcept override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE uav() const override;

    virtual D3D12Resource* handle() const noexcept override { return m_buffer->handle(); }
    virtual std::size_t size() const noexcept override { return m_buffer->size(); }

private:
    std::unique_ptr<d3d12_default_buffer> m_buffer;

    D3D12_VERTEX_BUFFER_VIEW m_view;
    std::size_t m_srv_offset;
    std::size_t m_uav_offset;
};

class d3d12_vertex_buffer_dynamic : public d3d12_vertex_buffer
{
public:
    d3d12_vertex_buffer_dynamic(const vertex_buffer_desc& desc);
    virtual ~d3d12_vertex_buffer_dynamic() = default;

    virtual void* pointer() override { return m_buffer->pointer(); }
    virtual void upload(const void* data, std::size_t size, std::size_t offset) override;

    virtual const D3D12_VERTEX_BUFFER_VIEW& view() const noexcept override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE srv() const override;
    virtual D3D12_CPU_DESCRIPTOR_HANDLE uav() const override;

    virtual D3D12Resource* handle() const noexcept override { return m_buffer->handle(); }
    virtual std::size_t size() const noexcept override { return m_buffer->size(); }

private:
    std::unique_ptr<d3d12_upload_buffer> m_buffer;

    std::size_t m_size;
    bool m_frame_resource;

    std::vector<D3D12_VERTEX_BUFFER_VIEW> m_views;
    std::size_t m_srv_offset;
    std::size_t m_uav_offset;

    std::size_t m_current_index;
    std::size_t m_last_sync_frame;
};

class d3d12_index_buffer : public d3d12_resource
{
public:
    d3d12_index_buffer(std::size_t index_count);
    virtual ~d3d12_index_buffer() = default;

    virtual const D3D12_INDEX_BUFFER_VIEW& view() const noexcept = 0;
    std::size_t index_count() const noexcept { return m_index_count; }

private:
    std::size_t m_index_count;
};

class d3d12_index_buffer_default : public d3d12_index_buffer
{
public:
    d3d12_index_buffer_default(
        const index_buffer_desc& desc,
        D3D12GraphicsCommandList* command_list);
    virtual ~d3d12_index_buffer_default() = default;

    virtual const D3D12_INDEX_BUFFER_VIEW& view() const noexcept override;
    virtual D3D12Resource* handle() const noexcept override { return m_buffer->handle(); }
    virtual std::size_t size() const noexcept override { return m_buffer->size(); }

private:
    std::unique_ptr<d3d12_default_buffer> m_buffer;
    D3D12_INDEX_BUFFER_VIEW m_view;
};

class d3d12_index_buffer_dynamic : public d3d12_index_buffer
{
public:
    d3d12_index_buffer_dynamic(const index_buffer_desc& desc);

    virtual void* pointer() override { return m_buffer->pointer(); }
    virtual void upload(const void* data, std::size_t size, std::size_t offset) override;

    virtual const D3D12_INDEX_BUFFER_VIEW& view() const noexcept override;
    virtual D3D12Resource* handle() const noexcept override { return m_buffer->handle(); }
    virtual std::size_t size() const noexcept override { return m_buffer->size(); }

private:
    std::unique_ptr<d3d12_upload_buffer> m_buffer;

    std::size_t m_size;
    bool m_frame_resource;

    std::vector<D3D12_INDEX_BUFFER_VIEW> m_views;

    std::size_t m_current_index;
    std::size_t m_last_sync_frame;
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

    void delay_delete(d3d12_ptr<D3D12Resource> resource);
    void switch_frame_resources();

private:
    heap_list m_heaps;
    heap_list m_visible_heaps;

    d3d12_frame_resource<temporary_list> m_temporary;
};
} // namespace ash::graphics::d3d12
#pragma once

#include "graphics/render_device.hpp"
#include <string>
#include <unordered_map>

namespace violet
{
class geometry
{
public:
    geometry();
    virtual ~geometry();

    template <std::ranges::contiguous_range R>
    void add_attribute(
        std::string_view name,
        R&& attribute,
        rhi_buffer_flags flags = RHI_BUFFER_VERTEX)
    {
        add_attribute(
            name,
            attribute.data(),
            attribute.size() * sizeof(decltype(*attribute.data())),
            flags);
        m_vertex_count = static_cast<std::uint32_t>(attribute.size());
    }

    template <typename T>
    void add_attribute(
        std::string_view name,
        std::size_t size,
        rhi_buffer_flags flags = RHI_BUFFER_VERTEX)
    {
        add_attribute(name, nullptr, size * sizeof(T), flags);
        m_vertex_count = static_cast<std::uint32_t>(size);
    }

    template <std::ranges::contiguous_range R>
    void set_indexes(R&& indexes, rhi_buffer_flags flags = RHI_BUFFER_INDEX)
    {
        static constexpr std::size_t index_size = sizeof(decltype(*indexes.data()));
        set_indexes(indexes.data(), indexes.size() * index_size, index_size, flags);
        m_index_count = static_cast<std::uint32_t>(indexes.size());
    }

    rhi_buffer* get_vertex_buffer(std::string_view name);

    rhi_buffer* get_index_buffer() const noexcept
    {
        return m_index_buffer.get();
    }

    std::uint32_t get_vertex_count() const noexcept
    {
        return m_vertex_count;
    }

    std::uint32_t get_index_count() const noexcept
    {
        return m_index_count;
    }

    render_id get_id() const noexcept
    {
        return m_id;
    }

private:
    void add_attribute(
        std::string_view name,
        const void* data,
        std::size_t size,
        rhi_buffer_flags flags);
    void set_indexes(
        const void* data,
        std::size_t size,
        std::size_t index_size,
        rhi_buffer_flags flags);

    std::unordered_map<std::string, rhi_ptr<rhi_buffer>> m_vertex_buffers;
    rhi_ptr<rhi_buffer> m_index_buffer;

    std::uint32_t m_vertex_count{0};
    std::uint32_t m_index_count{0};

    render_id m_id{INVALID_RENDER_ID};
};
} // namespace violet
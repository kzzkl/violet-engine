#pragma once

#include "graphics/morph_target.hpp"
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
            {
                .data = attribute.data(),
                .size = attribute.size() * sizeof(decltype(*attribute.data())),
                .flags = flags,
            });
    }

    void add_attribute(std::string_view name, const rhi_buffer_desc& desc);

    void add_attribute(std::string_view name, rhi_buffer* buffer);

    template <std::ranges::contiguous_range R>
    void set_indexes(R&& indexes, rhi_buffer_flags flags = RHI_BUFFER_INDEX)
    {
        static constexpr std::size_t index_size = sizeof(decltype(*indexes.data()));
        set_indexes(indexes.data(), indexes.size() * index_size, index_size, flags);
    }

    template <typename T>
    void set_indexes(const T* data, std::size_t count, rhi_buffer_flags flags = RHI_BUFFER_INDEX)
    {
        static constexpr std::size_t index_size = sizeof(T);
        set_indexes(data, count * sizeof(T), sizeof(T), flags);
    }

    void set_indexes(rhi_buffer* buffer);

    rhi_buffer* get_vertex_buffer(std::string_view name) const;

    const std::unordered_map<std::string, rhi_buffer*>& get_vertex_buffers() const noexcept
    {
        return m_vertex_buffer_map;
    }

    rhi_buffer* get_index_buffer() const noexcept
    {
        return m_index_buffer;
    }

    void set_vertex_count(std::uint32_t vertex_count) noexcept
    {
        m_vertex_count = vertex_count;
    }

    std::uint32_t get_vertex_count() const noexcept
    {
        return m_vertex_count;
    }

    void set_index_count(std::uint32_t index_count) noexcept
    {
        m_index_count = index_count;
    }

    std::uint32_t get_index_count() const noexcept
    {
        return m_index_count;
    }

    void add_morph_target(std::string_view name, const std::vector<morph_element>& elements);

    std::size_t get_morph_target_count() const noexcept
    {
        return m_morph_target_buffer->get_morph_target_count();
    }

    std::size_t get_morph_index(std::string_view name) const
    {
        return m_morph_name_to_index.at(name.data());
    }

    morph_target_buffer* get_morph_target_buffer() const noexcept
    {
        return m_morph_target_buffer.get();
    }

    render_id get_id() const noexcept
    {
        return m_id;
    }

private:
    void set_indexes(
        const void* data,
        std::size_t size,
        std::size_t index_size,
        rhi_buffer_flags flags);

    std::unordered_map<std::string, rhi_buffer*> m_vertex_buffer_map;
    rhi_buffer* m_index_buffer{nullptr};

    std::vector<rhi_ptr<rhi_buffer>> m_buffers;

    std::uint32_t m_vertex_count{0};
    std::uint32_t m_index_count{0};

    std::unordered_map<std::string, std::size_t> m_morph_name_to_index;
    std::unique_ptr<morph_target_buffer> m_morph_target_buffer;

    render_id m_id{INVALID_RENDER_ID};
};
} // namespace violet
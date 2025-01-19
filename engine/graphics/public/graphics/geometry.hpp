#pragma once

#include "graphics/buffer.hpp"
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
        assert(m_vertex_buffer_map.find(name.data()) == m_vertex_buffer_map.end());

        auto buffer = std::make_unique<vertex_buffer>(attribute, flags);
        m_vertex_buffer_map[name.data()] = buffer.get();
        m_buffers.emplace_back(std::move(buffer));
    }

    void add_attribute(std::string_view name, const rhi_buffer_desc& desc);

    void add_attribute(std::string_view name, vertex_buffer* buffer);

    template <std::ranges::contiguous_range R>
    void set_indexes(R&& indexes, rhi_buffer_flags flags = RHI_BUFFER_INDEX)
    {
        assert(m_index_buffer == nullptr);

        auto buffer = std::make_unique<index_buffer>(indexes, flags);
        m_index_buffer = buffer.get();
        m_buffers.emplace_back(std::move(buffer));
    }

    void set_indexes(index_buffer* buffer);

    vertex_buffer* get_vertex_buffer(std::string_view name) const;

    const std::unordered_map<std::string, vertex_buffer*>& get_vertex_buffers() const noexcept
    {
        return m_vertex_buffer_map;
    }

    index_buffer* get_index_buffer() const noexcept
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

    std::unordered_map<std::string, vertex_buffer*> m_vertex_buffer_map;
    index_buffer* m_index_buffer{nullptr};

    std::vector<std::unique_ptr<raw_buffer>> m_buffers;

    std::uint32_t m_vertex_count{0};
    std::uint32_t m_index_count{0};

    std::unordered_map<std::string, std::size_t> m_morph_name_to_index;
    std::unique_ptr<morph_target_buffer> m_morph_target_buffer;

    render_id m_id{INVALID_RENDER_ID};
};
} // namespace violet
#pragma once

#include "graphics/morph_target.hpp"
#include "graphics/render_device.hpp"
#include "math/box.hpp"
#include <array>
#include <string>
#include <unordered_map>

namespace violet
{
enum geometry_buffer_type
{
    GEOMETRY_BUFFER_POSITION,
    GEOMETRY_BUFFER_NORMAL,
    GEOMETRY_BUFFER_TANGENT,
    GEOMETRY_BUFFER_TEXCOORD,
    GEOMETRY_BUFFER_CUSTOM_0,
    GEOMETRY_BUFFER_CUSTOM_1,
    GEOMETRY_BUFFER_CUSTOM_2,
    GEOMETRY_BUFFER_CUSTOM_3,
    GEOMETRY_BUFFER_INDEX,
    GEOMETRY_BUFFER_COUNT,
};

class geometry
{
public:
    static constexpr std::size_t max_custom_attribute = 4;

    geometry();
    virtual ~geometry();

    void set_position(std::span<const vec3f> position);
    void set_position_shared(geometry* src_geometry);
    std::span<const vec3f> get_position() const noexcept;

    void set_normal(std::span<const vec3f> normal);
    void set_normal_shared(geometry* src_geometry);
    std::span<const vec3f> get_normal() const noexcept;

    void set_tangent(std::span<const vec4f> tangent);
    void set_tangent_shared(geometry* src_geometry);
    std::span<const vec4f> get_tangent() const noexcept;

    void set_texcoord(std::span<const vec2f> texcoord);
    void set_texcoord_shared(geometry* src_geometry);
    std::span<const vec2f> get_texcoord() const noexcept;

    template <std::ranges::contiguous_range R>
    void set_custom(std::size_t index, R&& attribute)
    {
        using type = decltype(*attribute.data());

        set_buffer(
            static_cast<geometry_buffer_type>(GEOMETRY_BUFFER_CUSTOM_0 + index),
            attribute.data(),
            attribute.size() * sizeof(type),
            sizeof(type));
    }

    void set_custom_shared(std::size_t index, geometry* src_geometry);

    void set_indexes(std::span<const std::uint32_t> indexes);
    void set_indexes_shared(geometry* src_geometry);
    std::span<const std::uint32_t> get_indexes() const noexcept;

    std::uint32_t get_vertex_count() const noexcept
    {
        return m_vertex_count;
    }

    std::uint32_t get_index_count() const noexcept
    {
        return m_index_count;
    }

    void add_morph_target(std::string_view name, const std::vector<morph_element>& elements);

    std::size_t get_morph_target_count() const noexcept
    {
        return m_morph_target_buffer == nullptr ? 0 :
                                                  m_morph_target_buffer->get_morph_target_count();
    }

    std::size_t get_morph_index(std::string_view name) const
    {
        return m_morph_name_to_index.at(name.data());
    }

    morph_target_buffer* get_morph_target_buffer() const noexcept
    {
        return m_morph_target_buffer.get();
    }

    void set_additional_buffer(
        std::string_view name,
        const void* data,
        std::size_t size,
        rhi_buffer_flags flags);

    raw_buffer* get_additional_buffer(std::string_view name) const;

    box3f get_aabb() const noexcept
    {
        return m_aabb;
    }

    render_id get_id() const noexcept
    {
        return m_id;
    }

    void update();

private:
    struct geometry_buffer
    {
        std::vector<std::uint8_t> buffer;
        std::size_t stride;

        geometry* src_geometry{nullptr};

        bool dirty{false};
    };

    void set_buffer(
        geometry_buffer_type type,
        const void* data,
        std::size_t size,
        std::size_t stride);
    void set_buffer_shared(geometry_buffer_type type, geometry* src_geometry);

    template <typename T>
    std::span<const T> get_buffer(geometry_buffer_type type) const
    {
        const auto& geometry_buffer = m_geometry_buffers[type];
        assert(geometry_buffer.stride == sizeof(T));

        if (geometry_buffer.src_geometry != nullptr)
        {
            return geometry_buffer.src_geometry->get_buffer<T>(type);
        }

        return {
            reinterpret_cast<const T*>(geometry_buffer.buffer.data()),
            type == GEOMETRY_BUFFER_INDEX ? m_index_count : m_vertex_count,
        };
    }

    void mark_dirty();

    std::array<geometry_buffer, GEOMETRY_BUFFER_COUNT> m_geometry_buffers;

    std::size_t m_vertex_count;
    std::size_t m_index_count;

    std::unordered_map<std::string, std::unique_ptr<raw_buffer>> m_additional_buffers;

    std::unordered_map<std::string, std::size_t> m_morph_name_to_index;
    std::unique_ptr<morph_target_buffer> m_morph_target_buffer;

    box3f m_aabb;

    render_id m_id{INVALID_RENDER_ID};

    bool m_dirty{false};
};
} // namespace violet
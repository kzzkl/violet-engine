#pragma once

#include "common/container.hpp"
#include "graphics/cluster.hpp"
#include "graphics/distance_field/distance_field.hpp"
#include "graphics/morph_target.hpp"
#include "graphics/render_device.hpp"
#include <array>

namespace violet
{
enum geometry_buffer_type
{
    GEOMETRY_BUFFER_POSITION,
    GEOMETRY_BUFFER_NORMAL,
    GEOMETRY_BUFFER_TANGENT,
    GEOMETRY_BUFFER_UV,
    GEOMETRY_BUFFER_CUSTOM_0,
    GEOMETRY_BUFFER_CUSTOM_1,
    GEOMETRY_BUFFER_CUSTOM_2,
    GEOMETRY_BUFFER_CUSTOM_3,
    GEOMETRY_BUFFER_INDEX,
    GEOMETRY_BUFFER_COUNT,
};

enum geometry_flag
{
    GEOMETRY_FLAG_NONE,
    GEOMETRY_FLAG_SDF,
    GEOMETRY_FLAG_BLAS,
    GEOMETRY_FLAG_MORPH,
};
using geometry_flags = std::uint32_t;

class geometry_sdf
{
public:
    void set_distance_field(const distance_field& distance_field)
    {
        m_distance_field = distance_field;
    }

    box3f get_volume_bounds() const noexcept
    {
        return m_distance_field.volume_bounds;
    }

    render_id get_id() const noexcept
    {
        return m_distance_field_id;
    }

private:
    distance_field m_distance_field;
    render_id m_distance_field_id{INVALID_RENDER_ID};

    friend class geometry;
};

class geometry_blas
{
public:
    rhi_acceleration_structure* get_blas() const noexcept
    {
        return m_blas.get();
    }

private:
    rhi_ptr<rhi_acceleration_structure> m_blas;

    friend class geometry;
};

class geometry_morph
{
public:
    void add_morph_target(std::string_view name, const std::vector<morph_element>& elements)
    {
        if (m_morph_target_buffer == nullptr)
        {
            m_morph_target_buffer = std::make_unique<morph_target_buffer>();
        }

        m_morph_name_to_index.insert_or_assign(
            std::string(name),
            m_morph_target_buffer->get_morph_target_count());

        m_morph_target_buffer->add_morph_target(elements);
    }

    std::size_t get_morph_target_count() const noexcept
    {
        return m_morph_target_buffer == nullptr ? 0 :
                                                  m_morph_target_buffer->get_morph_target_count();
    }

    std::size_t get_morph_index(std::string_view name) const
    {
        auto iter = m_morph_name_to_index.find(name);
        if (iter == m_morph_name_to_index.end())
        {
            throw std::runtime_error("Morph target not found.");
        }

        return iter->second;
    }

    morph_target_buffer* get_morph_target_buffer() const noexcept
    {
        return m_morph_target_buffer.get();
    }

private:
    string_map<std::size_t> m_morph_name_to_index;
    std::unique_ptr<morph_target_buffer> m_morph_target_buffer;
};

class geometry
{
public:
    static constexpr std::size_t max_custom_attribute = 4;

    struct submesh
    {
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;

        std::vector<cluster> clusters;
        std::vector<cluster_node> cluster_nodes;

        bool has_cluster() const noexcept
        {
            return !clusters.empty() && !cluster_nodes.empty();
        }

        std::uint32_t get_draw_call_count() const noexcept
        {
            return has_cluster() ? static_cast<std::uint32_t>(clusters.size()) : 1;
        }
    };

    geometry(geometry_flags flags = GEOMETRY_FLAG_NONE);
    geometry(const geometry&) = delete;
    virtual ~geometry();

    geometry& operator=(const geometry&) = delete;

    void set_positions(std::span<const vec3f> positions);
    void set_positions_shared(geometry* src_geometry);
    std::span<const vec3f> get_positions() const noexcept;

    void set_normals(std::span<const vec3f> normals);
    void set_normals_shared(geometry* src_geometry);
    std::span<const vec3f> get_normals() const noexcept;

    void set_tangents(std::span<const vec4f> tangents);
    void set_tangents_shared(geometry* src_geometry);
    std::span<const vec4f> get_tangents() const noexcept;

    void set_uvs(std::span<const vec2f> uvs);
    void set_uvs_shared(geometry* src_geometry);
    std::span<const vec2f> get_uvs() const noexcept;

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

    std::uint32_t add_submesh(
        std::uint32_t vertex_offset,
        std::uint32_t index_offset,
        std::uint32_t index_count);

    std::uint32_t add_submesh(
        std::span<const cluster> clusters,
        std::span<const cluster_node> cluster_nodes);

    void clear_submeshes();

    const submesh& get_submesh(std::uint32_t submesh_index) const noexcept
    {
        return m_submeshes[submesh_index];
    }

    const std::vector<submesh>& get_submeshes() const noexcept
    {
        return m_submeshes;
    }

    void set_additional_buffer(
        std::string_view name,
        const void* data,
        std::size_t size,
        rhi_buffer_flags flags);

    raw_buffer* get_additional_buffer(std::string_view name) const;

    template <typename F>
    void patch_sdf(F&& fn)
    {
        if (m_sdf != nullptr)
        {
            fn(*m_sdf);
            mark_dirty(DIRTY_FLAG_DISTANCE_FIELD);
        }
    }

    const geometry_sdf* get_sdf() const noexcept
    {
        return m_sdf.get();
    }

    const geometry_blas* get_blas() const noexcept
    {
        return m_blas.get();
    }

    template <typename F>
    void patch_morph(F&& fn)
    {
        if (m_morph != nullptr)
        {
            fn(*m_morph);
        }
    }

    geometry_morph* get_morph() const noexcept
    {
        return m_morph.get();
    }

    render_id get_geometry_id() const noexcept
    {
        return m_geometry_id;
    }

    render_id get_submesh_id(std::uint32_t submesh_index) const
    {
        return m_submesh_ids[submesh_index];
    }

    box3f get_bounding_box(std::uint32_t submesh_index) const;

    sphere3f get_bounding_sphere(std::uint32_t submesh_index) const;

    void update();

private:
    enum dirty_flag
    {
        DIRTY_FLAG_NONE = 0,
        DIRTY_FLAG_BUFFER = 1 << 0,
        DIRTY_FLAG_SUBMESH = 1 << 1,
        DIRTY_FLAG_DISTANCE_FIELD = 1 << 2,
        DIRTY_FLAG_BLAS = 1 << 3,
    };
    using dirty_flags = std::uint32_t;

    struct geometry_buffer
    {
        std::vector<std::uint8_t> buffer;
        std::size_t stride{0};

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

    void update_buffer();
    void update_submesh();
    void update_distance_field();
    void update_blas();

    void mark_dirty(dirty_flags dirty_flags);

    std::array<geometry_buffer, GEOMETRY_BUFFER_COUNT> m_geometry_buffers;

    std::uint32_t m_vertex_count{0};
    std::uint32_t m_index_count{0};

    string_map<std::unique_ptr<raw_buffer>> m_additional_buffers;

    std::vector<submesh> m_submeshes;
    std::vector<render_id> m_submesh_ids;

    std::unique_ptr<geometry_sdf> m_sdf;
    std::unique_ptr<geometry_blas> m_blas;
    std::unique_ptr<geometry_morph> m_morph;

    render_id m_geometry_id{INVALID_RENDER_ID};

    dirty_flags m_dirty_flags{DIRTY_FLAG_NONE};
};
} // namespace violet
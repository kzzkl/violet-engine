#pragma once

#include "graphics/morph_target.hpp"
#include "graphics/render_device.hpp"
#include "math/box.hpp"
#include "math/sphere.hpp"
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
    struct submesh
    {
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;
    };

    struct cluster
    {
        std::uint32_t index_offset;
        std::uint32_t index_count;

        box3f bounding_box;
        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float lod_error;

        sphere3f parent_lod_bounds;
        float parent_lod_error;

        std::uint32_t lod;

        std::uint32_t children_offset;
        std::uint32_t children_count;
    };

    struct cluster_bvh_node
    {
        sphere3f bounding_sphere;

        sphere3f lod_bounds;
        float min_lod_error;
        float max_parent_lod_error;

        bool is_leaf;
        std::vector<std::uint32_t> children;
    };

    static constexpr std::size_t max_custom_attribute = 4;

    geometry();
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

    void set_texcoords(std::span<const vec2f> texcoords);
    void set_texcoords_shared(geometry* src_geometry);
    std::span<const vec2f> get_texcoords() const noexcept;

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

    void set_submesh(
        std::uint32_t submesh_index,
        std::uint32_t vertex_offset,
        std::uint32_t index_offset,
        std::uint32_t index_count);

    void clear_submeshes();

    const std::vector<submesh>& get_submeshes() const noexcept
    {
        return m_submeshes;
    }

    void generate_clusters();

    void set_clusters(std::span<const cluster> clusters)
    {
        m_clusters.assign(clusters.begin(), clusters.end());
    }

    const std::vector<cluster>& get_clusters() const noexcept
    {
        return m_clusters;
    }

    const std::vector<cluster_bvh_node>& get_cluster_bvh_nodes() const noexcept
    {
        return m_cluster_bvh_nodes;
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

    render_id get_id() const noexcept
    {
        return m_id;
    }

    render_id get_submesh_id(std::uint32_t submesh_index) const
    {
        return m_submesh_infos[submesh_index].submesh_id;
    }

    void update();

private:
    struct geometry_buffer
    {
        std::vector<std::uint8_t> buffer;
        std::size_t stride{0};

        geometry* src_geometry{nullptr};
        bool dirty{false};
    };

    struct submesh_info
    {
        render_id submesh_id{INVALID_RENDER_ID};
        std::uint32_t cluster_offset;
        std::uint32_t cluster_count;

        bool dirty;
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

    void update_cluster();
    void update_submesh();
    void update_buffer();

    void mark_dirty();

    std::array<geometry_buffer, GEOMETRY_BUFFER_COUNT> m_geometry_buffers;

    std::uint32_t m_vertex_count{0};
    std::uint32_t m_index_count{0};

    std::unordered_map<std::string, std::unique_ptr<raw_buffer>> m_additional_buffers;

    std::vector<submesh> m_submeshes;
    std::vector<submesh_info> m_submesh_infos;

    std::vector<cluster> m_clusters;
    std::vector<cluster_bvh_node> m_cluster_bvh_nodes;

    std::unordered_map<std::string, std::size_t> m_morph_name_to_index;
    std::unique_ptr<morph_target_buffer> m_morph_target_buffer;

    render_id m_id{INVALID_RENDER_ID};

    bool m_dirty{false};
};
} // namespace violet
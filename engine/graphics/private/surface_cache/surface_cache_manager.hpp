#pragma once

#include "common/allocator.hpp"
#include "graphics/geometry.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/material.hpp"
#include "graphics/render_graph/render_graph.hpp"
#include "surface_cache/surface_cache_renderer.hpp"

namespace violet
{
class surface_cache_manager
{
public:
    surface_cache_manager();

    render_id add_surface_cache(
        geometry* geometry,
        std::span<std::pair<std::uint32_t, material*>> materials);
    void remove_surface_cache(render_id surface_cache_id);

    void upload(gpu_buffer_uploader* uploader);

    void render(render_graph& graph);

    rhi_buffer* get_surface_cache_buffer() const
    {
        return m_surface_caches.get_buffer()->get_rhi();
    }

    rhi_texture* get_albedo_buffer() const
    {
        return m_albedo_buffer.get();
    }

    rhi_texture* get_depth_buffer() const
    {
        return m_depth_buffer.get();
    }

private:
    struct surface_cache_key
    {
        geometry* geometry;
        std::vector<std::pair<std::uint32_t, material*>> materials;

        bool operator==(const surface_cache_key& other) const
        {
            return geometry == other.geometry && materials == other.materials;
        }
    };

    struct surface_cache_hash
    {
        std::size_t operator()(const surface_cache_key& key) const
        {
            return hash::combine(
                std::hash<void*>()(key.geometry),
                hash::xx_hash(
                    key.materials.data(),
                    key.materials.size() * sizeof(std::pair<std::uint32_t, material*>)));
        }
    };

    enum surface_flag
    {
        SURFACE_FLAG_NONE = 0,
        SURFACE_FLAG_RESIDENT = 1 << 1,
        SURFACE_FLAG_VALID = 1 << 2,
        SURFACE_FLAG_SUB_PAGE = 1 << 3,
        SURFACE_FLAG_MULTI_PAGE = 1 << 4,
    };
    using surface_flags = std::uint8_t;

    struct surface_cache
    {
        struct gpu_type
        {
            std::uint32_t extent;
            std::uint32_t page_count;
            std::array<std::uint32_t, SURFACE_CACHE_FACE_MAX_PAGE_COUNT> pages;
        };

        surface_flags flags{SURFACE_FLAG_NONE};

        buffer_allocation allocation;

        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t page_count_x;
        std::uint32_t page_count_y;

        std::uint32_t reference_count;

        union
        {
            struct
            {
                std::uint32_t sub_page;
            } sub_page;

            struct
            {
                std::uint32_t page_count;
            } multi_page;
        };

        bool is_valid() const noexcept
        {
            return flags != SURFACE_FLAG_NONE;
        }

        bool is_resident() const noexcept
        {
            return (flags & SURFACE_FLAG_RESIDENT) != 0;
        }
    };

    static constexpr surface_cache invalid_surface_cache = {.flags = 0};

    struct sub_page_allocation
    {
        std::uint32_t page;

        std::uint32_t width;
        std::uint32_t height;

        std::uint32_t sub_page_count;
        std::vector<std::uint32_t> free_sub_pages;
    };

    surface_cache allocate_sub_page(std::uint32_t width, std::uint32_t height);
    surface_cache allocate_multi_page(std::uint32_t width, std::uint32_t height);

    std::uint32_t allocate_page();

    std::vector<sub_page_allocation> m_sub_allocations;

    std::vector<std::uint32_t> m_page_table;
    buffer_allocator m_page_table_allocator;

    std::vector<std::uint32_t> m_free_pages;

    std::unordered_map<surface_cache_key, std::uint32_t, surface_cache_hash> m_surface_cache_map;
    gpu_block_sparse_array<surface_cache> m_surface_caches;

    struct render_queue_item
    {
        std::uint32_t surface_cache_id;
        geometry* geometry;
        std::vector<std::pair<std::uint32_t, material*>> materials;
        std::array<surface_cache_render_item::face, SURFACE_CACHE_FACE_COUNT> faces;
    };
    std::vector<render_queue_item> m_render_queue;

    rhi_ptr<rhi_texture> m_albedo_buffer;
    rhi_ptr<rhi_texture> m_depth_buffer;
};
} // namespace violet
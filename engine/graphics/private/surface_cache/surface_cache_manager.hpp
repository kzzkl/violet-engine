#pragma once

#include "common/allocator.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
class surface_cache_manager
{
public:
    surface_cache_manager();

    render_id add_surface_cache(
        std::uint32_t width,
        std::uint32_t height,
        geometry* geometry,
        std::span<std::pair<std::uint32_t, material*>> materials);
    void remove_surface_cache(render_id surface_cache_id);

    void render(render_graph& graph);

    rhi_texture* get_depth_buffer() const
    {
        return m_depth_buffer.get();
    }

private:
    static constexpr std::uint32_t atlas_resolution = 4096;
    static constexpr std::uint32_t page_resolution = 126;
    static constexpr std::uint32_t page_count =
        (atlas_resolution / page_resolution) * (atlas_resolution / page_resolution);

    struct surface_cache_key
    {
        std::uint32_t width;
        std::uint32_t height;
        geometry* geometry;
        std::vector<std::pair<std::uint32_t, material*>> materials;

        bool operator==(const surface_cache_key& other) const
        {
            return width == other.width && height == other.height && geometry == other.geometry &&
                   materials == other.materials;
        }
    };

    struct surface_cache_hash
    {
        std::size_t operator()(const surface_cache_key& key) const
        {
            std::size_t hash = hash::combine(
                std::hash<std::uint32_t>()(key.width),
                std::hash<std::uint32_t>()(key.height));
            hash = hash::combine(hash, std::hash<void*>()(key.geometry));
            hash = hash::combine(
                hash,
                hash::xx_hash(
                    key.materials.data(),
                    key.materials.size() * sizeof(std::pair<std::uint32_t, material*>)));
            return hash;
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
            std::uint32_t page_offset;
            std::uint32_t page_count;
            vec2f uv_offset;
        };

        surface_flags flags{SURFACE_FLAG_NONE};

        buffer_allocation allocation;

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
    std::vector<surface_cache> m_surface_caches;
    index_allocator m_surface_cache_allocator;

    struct render_queue_item
    {
        std::uint32_t surface_cache_id;
        std::uint32_t width;
        std::uint32_t height;
        geometry* geometry;
        std::vector<std::pair<std::uint32_t, material*>> materials;
    };
    std::vector<render_queue_item> m_render_queue;

    rhi_ptr<rhi_texture> m_depth_buffer;
};
} // namespace violet
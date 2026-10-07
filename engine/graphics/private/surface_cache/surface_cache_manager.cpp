#include "surface_cache/surface_cache_manager.hpp"
#include "gpu_buffer_uploader.hpp"
#include "math/math.hpp"
#include "surface_cache/surface_cache_common.hpp"
#include <algorithm>
#include <cstddef>
#include <numeric>

namespace violet
{
namespace
{
[[nodiscard]] vec2u get_atlas_position(std::uint32_t page)
{
    return {
        (page % SURFACE_CACHE_PAGE_COUNT_X) * SURFACE_CACHE_PAGE_RESOLUTION,
        (page / SURFACE_CACHE_PAGE_COUNT_X) * SURFACE_CACHE_PAGE_RESOLUTION,
    };
}
} // namespace

surface_cache_manager::surface_cache_manager()
    : m_page_table_allocator(static_cast<std::size_t>(SURFACE_CACHE_MAX_PAGE_COUNT)),
      m_surface_caches(SURFACE_CACHE_MAX_PAGE_COUNT)
{
    auto& device = render_device::instance();

    m_albedo_buffer = device.create_texture({
        .extent =
            {
                .width = SURFACE_CACHE_ATLAS_RESOLUTION,
                .height = SURFACE_CACHE_ATLAS_RESOLUTION,
                .depth = 1,
            },
        .format = RHI_FORMAT_R8G8B8A8_UNORM,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST,
        .level_count = 1,
        .layer_count = 1,
        .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
    });

    m_depth_buffer = device.create_texture({
        .extent =
            {
                .width = SURFACE_CACHE_ATLAS_RESOLUTION,
                .height = SURFACE_CACHE_ATLAS_RESOLUTION,
                .depth = 1,
            },
        .format = RHI_FORMAT_D32_FLOAT,
        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_DEPTH_STENCIL,
        .level_count = 1,
        .layer_count = 1,
        .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
    });

    m_free_pages.resize(SURFACE_CACHE_PAGE_COUNT);
    std::iota(m_free_pages.begin(), m_free_pages.end(), 0);

    m_page_table.resize(SURFACE_CACHE_PAGE_COUNT);
}

render_id surface_cache_manager::add_surface_cache(
    geometry* geometry,
    std::span<std::pair<std::uint32_t, material*>> materials)
{
    surface_cache_key key = {
        .geometry = geometry,
        .materials =
            std::vector<std::pair<std::uint32_t, material*>>(materials.begin(), materials.end()),
    };

    auto iter = m_surface_cache_map.find(key);
    if (iter != m_surface_cache_map.end())
    {
        ++m_surface_caches[iter->second].reference_count;
        return iter->second;
    }

    vec3f bounds_extent =
        box::get_extent(geometry->get_distance_field().volume_bounds) * SURFACE_CACHE_TEXEL_DENSITY;
    std::uint32_t extent_x = std::min(
        math::next_power_of_two(static_cast<std::uint32_t>(bounds_extent.x)),
        SURFACE_CACHE_FACE_MAX_RESOLUTION);
    std::uint32_t extent_y = std::min(
        math::next_power_of_two(static_cast<std::uint32_t>(bounds_extent.y)),
        SURFACE_CACHE_FACE_MAX_RESOLUTION);
    std::uint32_t extent_z = std::min(
        math::next_power_of_two(static_cast<std::uint32_t>(bounds_extent.z)),
        SURFACE_CACHE_FACE_MAX_RESOLUTION);

    std::array<vec2u, 3> surface_cache_extent;
    surface_cache_extent[0].x = extent_z;
    surface_cache_extent[0].y = extent_x;
    surface_cache_extent[1].x = extent_x;
    surface_cache_extent[1].y = extent_z;
    surface_cache_extent[2].x = extent_x;
    surface_cache_extent[2].y = extent_y;

    render_id surface_cache_id = m_surface_caches.add(SURFACE_CACHE_FACE_COUNT);

    std::array<surface_cache_render_item::face, SURFACE_CACHE_FACE_COUNT> faces;

    for (std::uint32_t i = 0; i < SURFACE_CACHE_FACE_COUNT; ++i)
    {
        surface_cache surface_cache;

        const auto& extent = surface_cache_extent[i / 2];

        auto& face = faces[i];
        face.width = extent.x;
        face.height = extent.y;

        bool sub_page =
            extent.x <= SURFACE_CACHE_PAGE_RESOLUTION && extent.y <= SURFACE_CACHE_PAGE_RESOLUTION;

        if (sub_page)
        {
            surface_cache = allocate_sub_page(extent.x, extent.y);
        }
        else
        {
            surface_cache = allocate_multi_page(extent.x, extent.y);
        }

        if (!surface_cache.is_valid())
        {
            m_surface_caches.remove(surface_cache_id);
            return INVALID_RENDER_ID;
        }

        if (sub_page)
        {
            std::uint32_t page = m_page_table[surface_cache.allocation.offset];
            std::uint32_t sub_page_count_x = SURFACE_CACHE_PAGE_RESOLUTION / extent.x;
            std::uint32_t sub_page_x = surface_cache.sub_page.sub_page % sub_page_count_x;
            std::uint32_t sub_page_y = surface_cache.sub_page.sub_page / sub_page_count_x;

            vec2u page_position = get_atlas_position(page);

            face.page_src_coords.emplace_back(0, 0);
            face.page_dst_coords.emplace_back(
                page_position.x + (sub_page_x * extent.x),
                page_position.y + (sub_page_y * extent.y));
            face.page_extents.emplace_back(extent.x, extent.y);
        }
        else
        {
            std::uint32_t page_count_x = extent.x / SURFACE_CACHE_PAGE_RESOLUTION;
            std::uint32_t page_count_y = extent.y / SURFACE_CACHE_PAGE_RESOLUTION;

            for (std::uint32_t y = 0; y < page_count_y; ++y)
            {
                for (std::uint32_t x = 0; x < page_count_x; ++x)
                {
                    vec2u page_position = get_atlas_position(
                        m_page_table[surface_cache.allocation.offset + (y * page_count_x) + x]);

                    face.page_src_coords.emplace_back(
                        x * SURFACE_CACHE_PAGE_RESOLUTION,
                        y * SURFACE_CACHE_PAGE_RESOLUTION);
                    face.page_dst_coords.push_back(page_position);
                    face.page_extents.emplace_back(
                        std::min(
                            SURFACE_CACHE_PAGE_RESOLUTION,
                            extent.x - (x * SURFACE_CACHE_PAGE_RESOLUTION)),
                        std::min(
                            SURFACE_CACHE_PAGE_RESOLUTION,
                            extent.y - (y * SURFACE_CACHE_PAGE_RESOLUTION)));
                }
            }
        }

        surface_cache.width = static_cast<std::uint16_t>(extent.x);
        surface_cache.height = static_cast<std::uint16_t>(extent.y);
        surface_cache.page_count_x = static_cast<std::uint16_t>(
            std::max<std::uint32_t>(1, extent.x / SURFACE_CACHE_PAGE_RESOLUTION));
        surface_cache.page_count_y = static_cast<std::uint16_t>(
            std::max<std::uint32_t>(1, extent.y / SURFACE_CACHE_PAGE_RESOLUTION));

        surface_cache.flags |= SURFACE_FLAG_VALID;
        surface_cache.reference_count = 1;

        m_surface_caches[surface_cache_id + i] = surface_cache;
    }

    m_surface_cache_map.emplace(key, surface_cache_id);

    m_render_queue.push_back({
        .surface_cache_id = surface_cache_id,
        .geometry = geometry,
        .materials =
            std::vector<std::pair<std::uint32_t, material*>>(materials.begin(), materials.end()),
        .faces = faces,
    });

    return surface_cache_id;
}

void surface_cache_manager::remove_surface_cache(render_id surface_cache_id)
{
    if (--m_surface_caches[surface_cache_id].reference_count > 0)
    {
        return;
    }

    for (std::uint32_t i = 0; i < SURFACE_CACHE_FACE_COUNT; ++i)
    {
        auto& surface_cache = m_surface_caches[surface_cache_id + i];

        if (surface_cache.flags & SURFACE_FLAG_SUB_PAGE)
        {
            auto iter = std::ranges::find_if(
                m_sub_allocations,
                [&](const sub_page_allocation& allocation)
                {
                    return allocation.page == m_page_table[surface_cache.allocation.offset];
                });

            iter->free_sub_pages.push_back(surface_cache.sub_page.sub_page);

            if (iter->sub_page_count == iter->free_sub_pages.size())
            {
                m_free_pages.push_back(iter->page);
                m_sub_allocations.erase(iter);
            }
        }
        else if (surface_cache.flags & SURFACE_FLAG_MULTI_PAGE)
        {
            for (std::uint32_t i = 0; i < surface_cache.multi_page.page_count; ++i)
            {
                m_free_pages.push_back(m_page_table[surface_cache.allocation.offset + i]);
            }
        }

        surface_cache.flags = SURFACE_FLAG_NONE;
        surface_cache.reference_count = 0;

        m_page_table_allocator.free(surface_cache.allocation);
    }

    m_surface_caches.remove(surface_cache_id);
}

void surface_cache_manager::upload(gpu_buffer_uploader* uploader)
{
    m_surface_caches.update(
        [&](const surface_cache& surface_cache) -> surface_cache::gpu_type
        {
            surface_cache::gpu_type data = {
                .extent = surface_cache.width | (surface_cache.height << 16),
                .page_count = surface_cache.page_count_x | (surface_cache.page_count_y << 16),
            };

            // The pages of a face are not necessarily contiguous in the atlas, so every page
            // stores its own atlas position. m_page_table is shared between all surface caches
            // and is indexed by the table allocation of this face.
            std::uint32_t page_count = surface_cache.page_count_x * surface_cache.page_count_y;
            for (std::uint32_t i = 0; i < page_count; ++i)
            {
                std::uint32_t page = m_page_table[surface_cache.allocation.offset + i];
                vec2u position = get_atlas_position(page);

                // A sub page is a rectangle smaller than a page placed inside a shared page. The
                // atlas position of the page is the position of its first sub page.
                if (surface_cache.flags & SURFACE_FLAG_SUB_PAGE)
                {
                    std::uint32_t sub_page_count_x =
                        SURFACE_CACHE_PAGE_RESOLUTION / surface_cache.width;

                    position.x +=
                        (surface_cache.sub_page.sub_page % sub_page_count_x) * surface_cache.width;
                    position.y +=
                        (surface_cache.sub_page.sub_page / sub_page_count_x) * surface_cache.height;
                }

                data.pages[i] = position.x | (position.y << 16);
            }

            return data;
        },
        [&](rhi_buffer* buffer, const void* data, std::size_t size, std::size_t offset)
        {
            rhi_buffer_region region = {
                .offset = offset,
                .size = size,
            };

            uploader->upload(
                buffer,
                data,
                size,
                region,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_ACCESS_SHADER_READ);
        });
}

void surface_cache_manager::render(render_graph& graph)
{
    if (m_render_queue.empty())
    {
        return;
    }

    std::vector<surface_cache_render_item> items;
    items.reserve(m_render_queue.size());

    for (const auto& item : m_render_queue)
    {
        items.push_back({
            .faces = item.faces,
            .geometry = item.geometry,
            .materials = item.materials,
        });
    }

    surface_cache_renderer renderer;
    renderer.render(graph, items, m_albedo_buffer.get(), m_depth_buffer.get());

    m_render_queue.clear();
}

surface_cache_manager::surface_cache surface_cache_manager::allocate_sub_page(
    std::uint32_t width,
    std::uint32_t height)
{
    std::uint32_t page = 0xFFFFFFFF;
    std::uint32_t sub_page = 0;

    for (auto& allocation : m_sub_allocations)
    {
        if (allocation.free_sub_pages.empty())
        {
            continue;
        }

        if (allocation.width == width && allocation.height == height)
        {
            page = allocation.page;
            sub_page = allocation.free_sub_pages.back();

            allocation.free_sub_pages.pop_back();
        }
    }

    if (page == 0xFFFFFFFF)
    {
        page = allocate_page();

        if (page == 0xFFFFFFFF)
        {
            return invalid_surface_cache;
        }

        std::uint32_t sub_page_count_x = SURFACE_CACHE_PAGE_RESOLUTION / width;
        std::uint32_t sub_page_count_y = SURFACE_CACHE_PAGE_RESOLUTION / height;
        std::uint32_t sub_page_count = sub_page_count_x * sub_page_count_y;

        sub_page_allocation allocation = {
            .page = page,
            .width = width,
            .height = height,
            .sub_page_count = sub_page_count,
        };

        allocation.free_sub_pages.reserve(sub_page_count);
        for (std::uint32_t i = 0; i < sub_page_count - 1; ++i)
        {
            allocation.free_sub_pages.push_back(i);
        }

        sub_page = sub_page_count - 1;

        m_sub_allocations.push_back(std::move(allocation));
    }

    surface_cache surface_cache = {
        .flags = SURFACE_FLAG_SUB_PAGE,
    };

    surface_cache.allocation = m_page_table_allocator.allocate(1);
    if (m_page_table.size() <= surface_cache.allocation.offset)
    {
        m_page_table.resize(surface_cache.allocation.offset + 1);
    }

    m_page_table[surface_cache.allocation.offset] = page;

    surface_cache.sub_page.sub_page = sub_page;

    return surface_cache;
}

surface_cache_manager::surface_cache surface_cache_manager::allocate_multi_page(
    std::uint32_t width,
    std::uint32_t height)
{
    std::uint32_t multi_page_count_x = width / SURFACE_CACHE_PAGE_RESOLUTION;
    std::uint32_t multi_page_count_y = height / SURFACE_CACHE_PAGE_RESOLUTION;
    std::uint32_t multi_page_count = multi_page_count_x * multi_page_count_y;

    if (m_free_pages.size() < multi_page_count)
    {
        return invalid_surface_cache;
    }

    surface_cache surface_cache = {
        .flags = SURFACE_FLAG_MULTI_PAGE,
    };

    surface_cache.allocation = m_page_table_allocator.allocate(multi_page_count);
    if (m_page_table.size() <= surface_cache.allocation.offset + multi_page_count)
    {
        m_page_table.resize(surface_cache.allocation.offset + multi_page_count + 1);
    }

    for (std::uint32_t i = 0; i < multi_page_count; ++i)
    {
        m_page_table[surface_cache.allocation.offset + i] = allocate_page();
    }

    surface_cache.multi_page.page_count = multi_page_count;

    return surface_cache;
}

std::uint32_t surface_cache_manager::allocate_page()
{
    if (m_free_pages.empty())
    {
        return 0xFFFFFFFF;
    }

    std::uint32_t page = m_free_pages.back();
    m_free_pages.pop_back();
    return page;
}
} // namespace violet
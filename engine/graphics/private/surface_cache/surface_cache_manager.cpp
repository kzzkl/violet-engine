#include "surface_cache/surface_cache_manager.hpp"
#include "math/math.hpp"
#include "surface_cache/surface_cache_renderer.hpp"
#include <algorithm>
#include <cstddef>
#include <numeric>

namespace violet
{
surface_cache_manager::surface_cache_manager()
    : m_page_table_allocator(static_cast<std::size_t>(page_count * 16))
{
    auto& device = render_device::instance();

    m_depth_buffer = device.create_texture({
        .extent = {.width = atlas_resolution, .height = atlas_resolution, .depth = 1},
        .format = RHI_FORMAT_D32_FLOAT,
        .flags = RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE,
        .level_count = 1,
        .layer_count = 1,
        .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
    });

    std::uint32_t page_count_x = atlas_resolution / page_resolution;
    std::uint32_t page_count_y = atlas_resolution / page_resolution;
    std::uint32_t page_count = page_count_x * page_count_y;

    m_free_pages.resize(page_count);
    std::iota(m_free_pages.begin(), m_free_pages.end(), 0);

    m_page_table.resize(page_count);
}

render_id surface_cache_manager::add_surface_cache(
    std::uint32_t width,
    std::uint32_t height,
    geometry* geometry,
    std::span<std::pair<std::uint32_t, material*>> materials)
{
    assert(math::is_power_of_two(width) && math::is_power_of_two(height));

    surface_cache_key key = {
        .width = width,
        .height = height,
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

    surface_cache surface_cache;

    if (width <= page_resolution && height <= page_resolution)
    {
        surface_cache = allocate_sub_page(width, height);
    }
    else
    {
        surface_cache = allocate_multi_page(width, height);
    }

    if (!surface_cache.is_valid())
    {
        return INVALID_RENDER_ID;
    }

    surface_cache.flags |= SURFACE_FLAG_VALID;
    surface_cache.reference_count = 1;

    render_id surface_cache_id = m_surface_cache_allocator.allocate();
    if (m_surface_caches.size() <= surface_cache_id)
    {
        m_surface_caches.resize(surface_cache_id + 1, {});
    }

    m_surface_caches[surface_cache_id] = surface_cache;
    m_surface_cache_map.emplace(key, surface_cache_id);

    m_render_queue.push_back({
        .surface_cache_id = surface_cache_id,
        .width = width,
        .height = height,
        .geometry = geometry,
        .materials =
            std::vector<std::pair<std::uint32_t, material*>>(materials.begin(), materials.end()),
    });

    return surface_cache_id;
}

void surface_cache_manager::remove_surface_cache(render_id surface_cache_id)
{
    auto& surface_cache = m_surface_caches[surface_cache_id];

    if (--surface_cache.reference_count > 0)
    {
        return;
    }

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

    m_page_table_allocator.free(surface_cache.allocation);
    m_surface_cache_allocator.free(surface_cache_id);
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
            .geometry = item.geometry,
            .materials = item.materials,
        });
    }

    surface_cache_renderer renderer;
    renderer.render(graph, items, nullptr, nullptr);

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
        sub_page = 0;

        if (page == 0xFFFFFFFF)
        {
            return invalid_surface_cache;
        }

        std::uint32_t sub_page_count_x = page_resolution / width;
        std::uint32_t sub_page_count_y = page_resolution / height;
        std::uint32_t sub_page_count = sub_page_count_x * sub_page_count_y;

        sub_page_allocation allocation = {
            .page = page,
            .width = width,
            .height = height,
            .sub_page_count = sub_page_count,
        };

        allocation.free_sub_pages.resize(sub_page_count);
        for (std::uint32_t i = 1; i < sub_page_count; ++i)
        {
            allocation.free_sub_pages[i] = i;
        }

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
    std::uint32_t multi_page_count_x = width / page_resolution;
    std::uint32_t multi_page_count_y = height / page_resolution;
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
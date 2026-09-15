#include "gpu_buffer_uploader.hpp"
#include "graphics/render_device.hpp"
#include <algorithm>

namespace violet
{
gpu_buffer_uploader::gpu_buffer_uploader(
    std::size_t staging_page_size,
    std::size_t staging_page_count)
    : m_current_staging_page_index(staging_page_count),
      m_staging_page_size(staging_page_size),
      m_staging_page_count(staging_page_count)
{
    m_active_staging_pages.resize(render_device::instance().get_frame_resource_count());
}

void gpu_buffer_uploader::tick()
{
    reset_active_staging_pages(render_device::instance().get_frame_resource_index());
    m_current_staging_page_index = m_staging_page_count;
}

void gpu_buffer_uploader::upload(
    rhi_buffer* buffer,
    const void* data,
    std::size_t size,
    const rhi_buffer_region& region,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access)
{
    assert(size == region.size);

    std::size_t pending_size = size;

    while (pending_size > 0)
    {
        auto& staging_page = allocate_staging_page();

        std::size_t reserve_size = staging_page.get_reserve_size();
        std::size_t upload_size = std::min(reserve_size, pending_size);

        m_buffer_requests.push_back({
            .src = staging_page.buffer.get(),
            .src_region =
                {
                    .offset = static_cast<std::uint32_t>(staging_page.offset),
                    .size = upload_size,
                },
            .dst = buffer,
            .dst_region =
                {
                    .offset = static_cast<std::uint32_t>(region.offset + size - pending_size),
                    .size = upload_size,
                },
        });

        staging_page.copy(data, upload_size);

        auto& [buffer_stages, buffer_access] = m_dst_buffers[buffer];
        buffer_stages |= stages;
        buffer_access |= access;

        data = static_cast<const std::uint8_t*>(data) + upload_size;
        pending_size -= upload_size;
    }
}

void gpu_buffer_uploader::upload(
    rhi_texture* texture,
    const void* data,
    std::size_t size,
    const rhi_texture_region& region,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access,
    rhi_texture_layout layout)
{
    assert(size < m_staging_page_size);

    auto& staging_page = allocate_staging_page(size);

    m_texture_requests.push_back({
        .src = staging_page.buffer.get(),
        .src_region =
            {
                .offset = static_cast<std::uint32_t>(staging_page.offset),
                .size = size,
            },
        .dst = texture,
        .dst_region = region,
        .layout = layout,
    });

    staging_page.copy(data, size);

    auto iter = m_dst_textures.find(texture);
    if (iter == m_dst_textures.end())
    {
        m_dst_textures[texture] = {
            .stages = stages,
            .access = access,
            .layout = layout,
        };
    }
    else
    {
        assert(iter->second.layout == layout);

        iter->second.stages |= stages;
        iter->second.access |= access;
    }
}

void gpu_buffer_uploader::record(rhi_command* command)
{
    if (!m_buffer_requests.empty())
    {
        record_buffer_request(command);
    }

    if (!m_texture_requests.empty())
    {
        record_texture_request(command);
    }
}

gpu_buffer_uploader::staging_page& gpu_buffer_uploader::allocate_staging_page(
    std::size_t expected_size)
{
    assert(expected_size <= m_staging_page_size);

    if (m_current_staging_page_index != m_staging_page_count &&
        m_staging_pages[m_current_staging_page_index].get_reserve_size() > expected_size)
    {
        return m_staging_pages[m_current_staging_page_index];
    }

    auto& device = render_device::instance();

    if (m_free_staging_pages.empty())
    {
        if (m_staging_pages.size() == m_staging_page_count)
        {
            flush();
        }
        else
        {
            m_free_staging_pages.push_back(m_staging_pages.size());

            rhi_buffer_desc staging_buffer_desc = {
                .data = nullptr,
                .size = m_staging_page_size,
                .flags = RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_HOST_VISIBLE,
            };

            m_staging_pages.emplace_back(
                staging_page{
                    .buffer = device.create_buffer(staging_buffer_desc),
                    .offset = 0,
                });
        }
    }

    m_current_staging_page_index = m_free_staging_pages.back();
    m_free_staging_pages.pop_back();

    m_active_staging_pages[device.get_frame_resource_index()].push_back(
        m_current_staging_page_index);

    return m_staging_pages[m_current_staging_page_index];
}

void gpu_buffer_uploader::flush()
{
    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    record(command);
    device.execute_sync(command);

    for (std::uint32_t i = 0; i < m_active_staging_pages.size(); ++i)
    {
        reset_active_staging_pages(i);
    }
}

void gpu_buffer_uploader::reset_active_staging_pages(std::uint32_t frame_resource_index)
{
    auto& active_staging_pages = m_active_staging_pages[frame_resource_index];

    for (std::size_t index : active_staging_pages)
    {
        m_staging_pages[index].offset = 0;
        m_free_staging_pages.push_back(index);
    }

    active_staging_pages.clear();
}

void gpu_buffer_uploader::record_buffer_request(rhi_command* command)
{
    std::vector<rhi_buffer_barrier> barriers;
    barriers.reserve(m_dst_buffers.size());

    for (auto& [buffer, sync] : m_dst_buffers)
    {
        barriers.push_back({
            .buffer = buffer,
            .src_stages = sync.stages | RHI_PIPELINE_STAGE_TRANSFER,
            .src_access = sync.access | RHI_ACCESS_TRANSFER_WRITE,
            .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
            .dst_access = RHI_ACCESS_TRANSFER_WRITE,
            .offset = 0,
            .size = buffer->get_size(),
        });
    }

    command->set_pipeline_barrier(
        barriers.data(),
        static_cast<std::uint32_t>(barriers.size()),
        nullptr,
        0);

    // merge the upload commands.
    std::ranges::sort(
        m_buffer_requests,
        [](const buffer_upload_request& a, const buffer_upload_request& b)
        {
            if (a.src != b.src)
            {
                return a.src < b.src;
            }

            if (a.src_region.offset != b.src_region.offset)
            {
                return a.src_region.offset < b.src_region.offset;
            }

            if (a.dst != b.dst)
            {
                return a.dst < b.dst;
            }

            if (a.dst_region.offset != b.dst_region.offset)
            {
                return a.dst_region.offset < b.dst_region.offset;
            }

            throw std::runtime_error("The uploaded memory buffers overlap.");
        });

    rhi_buffer* src = nullptr;
    rhi_buffer_region src_region = {};

    rhi_buffer* dst = nullptr;
    rhi_buffer_region dst_region = {};

    for (auto& request : m_buffer_requests)
    {
        if (request.src != src || request.dst != dst ||
            request.src_region.offset != src_region.offset + src_region.size ||
            request.dst_region.offset != dst_region.offset + dst_region.size)
        {
            if (src != nullptr)
            {
                command->copy_buffer(src, &src_region, dst, &dst_region, 1);
            }

            src = request.src;
            src_region = request.src_region;

            dst = request.dst;
            dst_region = request.dst_region;
        }
        else
        {
            src_region.size += request.src_region.size;
            dst_region.size += request.dst_region.size;
        }
    }

    if (src != nullptr)
    {
        command->copy_buffer(src, &src_region, dst, &dst_region, 1);
    }

    for (rhi_buffer_barrier& barrier : barriers)
    {
        std::swap(barrier.src_stages, barrier.dst_stages);
        std::swap(barrier.src_access, barrier.dst_access);
    }

    command->set_pipeline_barrier(
        barriers.data(),
        static_cast<std::uint32_t>(barriers.size()),
        nullptr,
        0);

    m_buffer_requests.clear();
    m_dst_buffers.clear();
}

void gpu_buffer_uploader::record_texture_request(rhi_command* command)
{
    std::vector<rhi_texture_barrier> barriers;
    barriers.reserve(m_dst_textures.size());

    for (auto& [texture, sync] : m_dst_textures)
    {
        barriers.push_back({
            .texture = texture,
            .src_stages = sync.stages | RHI_PIPELINE_STAGE_TRANSFER,
            .src_access = sync.access | RHI_ACCESS_TRANSFER_WRITE,
            .src_layout = sync.layout,
            .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
            .dst_access = RHI_ACCESS_TRANSFER_WRITE,
            .dst_layout = RHI_TEXTURE_LAYOUT_TRANSFER_DST,
            .level = 0,
            .level_count = texture->get_level_count(),
            .layer = 0,
            .layer_count = texture->get_layer_count(),
        });
    }

    command->set_pipeline_barrier(
        nullptr,
        0,
        barriers.data(),
        static_cast<std::uint32_t>(barriers.size()));

    rhi_buffer* src = nullptr;
    std::vector<rhi_buffer_region> src_regions;

    rhi_texture* dst = nullptr;
    std::vector<rhi_texture_region> dst_regions;

    for (auto& request : m_texture_requests)
    {
        if (request.src != src || request.dst != dst)
        {
            if (!src_regions.empty())
            {
                command->copy_buffer_to_texture(
                    src,
                    src_regions.data(),
                    dst,
                    dst_regions.data(),
                    src_regions.size());

                src_regions.clear();
                dst_regions.clear();
            }

            src = request.src;
            dst = request.dst;
        }

        src_regions.push_back(request.src_region);
        dst_regions.push_back(request.dst_region);
    }

    if (!src_regions.empty())
    {
        command->copy_buffer_to_texture(
            src,
            src_regions.data(),
            dst,
            dst_regions.data(),
            src_regions.size());

        src_regions.clear();
        dst_regions.clear();
    }
}
} // namespace violet
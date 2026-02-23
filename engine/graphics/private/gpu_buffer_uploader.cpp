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
    std::size_t offset,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access)
{
    std::size_t pending_size = size;

    while (pending_size > 0)
    {
        auto& staging_page = allocate_staging_page();

        std::size_t reserve_size = staging_page.get_reserve_size();
        std::size_t upload_size = std::min(reserve_size, pending_size);

        m_upload_commands.push_back({
            .src = staging_page.buffer.get(),
            .src_offset = static_cast<std::uint32_t>(staging_page.offset),
            .dst = buffer,
            .dst_offset = static_cast<std::uint32_t>(offset + size - pending_size),
            .size = upload_size,
        });

        staging_page.copy(data, upload_size);

        auto& [buffer_stages, buffer_access] = m_dst_buffers[buffer];
        buffer_stages |= stages;
        buffer_access |= access;

        data = static_cast<const std::uint8_t*>(data) + upload_size;
        pending_size -= upload_size;
    }
}

void gpu_buffer_uploader::record(rhi_command* command)
{
    if (m_upload_commands.empty())
    {
        return;
    }

    std::vector<rhi_buffer_barrier> barriers;
    barriers.reserve(m_dst_buffers.size());

    for (auto& [buffer, stages_access] : m_dst_buffers)
    {
        rhi_buffer_barrier barrier = {
            .buffer = buffer,
            .src_stages = stages_access.first | RHI_PIPELINE_STAGE_TRANSFER,
            .src_access = stages_access.second | RHI_ACCESS_TRANSFER_WRITE,
            .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
            .dst_access = RHI_ACCESS_TRANSFER_WRITE,
            .offset = 0,
            .size = buffer->get_size(),
        };

        barriers.push_back(barrier);
    }

    command->set_pipeline_barrier(
        barriers.data(),
        static_cast<std::uint32_t>(barriers.size()),
        nullptr,
        0);

    // merge the upload commands.
    std::ranges::sort(
        m_upload_commands,
        [](const upload_command& a, const upload_command& b)
        {
            if (a.src != b.src)
            {
                return a.src < b.src;
            }

            if (a.src_offset != b.src_offset)
            {
                return a.src_offset < b.src_offset;
            }

            if (a.dst != b.dst)
            {
                return a.dst < b.dst;
            }

            if (a.dst_offset != b.dst_offset)
            {
                return a.dst_offset < b.dst_offset;
            }

            throw std::runtime_error("The uploaded memory buffers overlap.");
        });

    rhi_buffer* src = nullptr;
    rhi_buffer_region src_region = {};

    rhi_buffer* dst = nullptr;
    rhi_buffer_region dst_region = {};

    for (auto& upload_command : m_upload_commands)
    {
        if (upload_command.src != src || upload_command.dst != dst ||
            upload_command.src_offset != src_region.offset + src_region.size ||
            upload_command.dst_offset != dst_region.offset + dst_region.size)
        {
            if (src != nullptr)
            {
                command->copy_buffer(src, src_region, dst, dst_region);
            }

            src = upload_command.src;
            src_region.offset = upload_command.src_offset;
            src_region.size = upload_command.size;

            dst = upload_command.dst;
            dst_region.offset = upload_command.dst_offset;
            dst_region.size = upload_command.size;
        }
        else
        {
            src_region.size += upload_command.size;
            dst_region.size += upload_command.size;
        }
    }

    if (src != nullptr)
    {
        command->copy_buffer(src, src_region, dst, dst_region);
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

    m_upload_commands.clear();
    m_dst_buffers.clear();
}

gpu_buffer_uploader::staging_page& gpu_buffer_uploader::allocate_staging_page()
{
    if (m_current_staging_page_index != m_staging_page_count &&
        m_staging_pages[m_current_staging_page_index].get_reserve_size() > 0)
    {
        return m_staging_pages[m_current_staging_page_index];
    }

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
                    .buffer = render_device::instance().create_buffer(staging_buffer_desc),
                    .offset = 0,
                });
        }
    }

    m_current_staging_page_index = m_free_staging_pages.back();
    m_free_staging_pages.pop_back();

    m_active_staging_pages[render_device::instance().get_frame_resource_index()].push_back(
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
} // namespace violet
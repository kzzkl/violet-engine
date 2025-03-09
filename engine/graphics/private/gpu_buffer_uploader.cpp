#include "gpu_buffer_uploader.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
gpu_buffer_uploader::gpu_buffer_uploader(
    std::size_t staging_page_size,
    std::size_t max_staging_pages)
    : m_staging_page_size(staging_page_size),
      m_max_staging_pages(max_staging_pages)
{
    m_active_staging_pages.resize(render_device::instance().get_frame_resource_count());
}

void gpu_buffer_uploader::tick()
{
    reset_active_staging_pages();
}

void gpu_buffer_uploader::upload(
    rhi_buffer* buffer,
    const void* data,
    std::size_t size,
    std::uint32_t offset,
    rhi_pipeline_stage_flags stages,
    rhi_access_flags access)
{
    std::size_t pending_size = size;

    while (pending_size > 0)
    {
        if (m_staging_pages.size() == m_max_staging_pages)
        {
            auto& [buffer_stages, buffer_access] = m_dst_buffers[buffer];
            buffer_stages |= stages;
            buffer_access |= access;

            flush();
        }

        auto& staging_page = allocate_staging_page();

        std::size_t reserve_size = staging_page.get_reserve_size();
        std::size_t upload_size = std::min(reserve_size, pending_size);

        m_upload_commands.push_back({
            .src = staging_page.buffer.get(),
            .src_offset = static_cast<std::uint32_t>(staging_page.offset),
            .dst = buffer,
            .dst_offset = offset + static_cast<std::uint32_t>(size - pending_size),
            .size = upload_size,
        });

        staging_page.copy(data, upload_size);

        data = static_cast<const std::uint8_t*>(data) + upload_size;
        pending_size -= upload_size;
    }

    auto& [buffer_stages, buffer_access] = m_dst_buffers[buffer];
    buffer_stages |= stages;
    buffer_access |= access;
}

void gpu_buffer_uploader::record(rhi_command* command)
{
    std::vector<rhi_buffer_barrier> barriers;
    barriers.reserve(m_dst_buffers.size());

    for (auto& [buffer, stages_access] : m_dst_buffers)
    {
        rhi_buffer_barrier barrier = {
            .buffer = buffer,
            .src_stages = stages_access.first,
            .src_access = stages_access.second,
            .dst_stages = RHI_PIPELINE_STAGE_TRANSFER,
            .dst_access = RHI_ACCESS_TRANSFER_WRITE,
            .offset = 0,
            .size = buffer->get_size(),
        };

        barriers.push_back(barrier);
    }

    command->set_pipeline_barrier(barriers.data(), barriers.size(), nullptr, 0);

    for (auto& upload : m_upload_commands)
    {
        rhi_buffer_region src_region = {
            .offset = upload.src_offset,
            .size = upload.size,
        };
        rhi_buffer_region dst_region = {
            .offset = upload.dst_offset,
            .size = upload.size,
        };

        command->copy_buffer(upload.src, src_region, upload.dst, dst_region);
    }

    for (rhi_buffer_barrier& barrier : barriers)
    {
        std::swap(barrier.src_stages, barrier.dst_stages);
        std::swap(barrier.src_access, barrier.dst_access);
    }

    command->set_pipeline_barrier(barriers.data(), barriers.size(), nullptr, 0);

    m_upload_commands.clear();
    m_dst_buffers.clear();
}

gpu_buffer_uploader::staging_page& gpu_buffer_uploader::allocate_staging_page()
{
    std::size_t index = 0;
    if (!m_staging_pages.empty() && m_staging_pages.back().offset != m_staging_page_size)
    {
        index = m_staging_pages.size() - 1;
    }
    else if (m_free_staging_pages.empty())
    {
        index = m_staging_pages.size();

        rhi_buffer_desc staging_buffer_desc = {
            .data = nullptr,
            .size = m_staging_page_size,
            .flags = RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_HOST_VISIBLE,
        };

        m_staging_pages.emplace_back(staging_page{
            .buffer = render_device::instance().create_buffer(staging_buffer_desc),
            .offset = 0,
        });
    }
    else
    {
        index = m_free_staging_pages.back();
        m_free_staging_pages.pop_back();
    }

    m_active_staging_pages[render_device::instance().get_frame_resource_index()].push_back(index);

    return m_staging_pages[index];
}

void gpu_buffer_uploader::flush()
{
    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();
    record(command);
    device.execute(command, true);

    reset_active_staging_pages();
}

void gpu_buffer_uploader::reset_active_staging_pages()
{
    auto& active_staging_pages =
        m_active_staging_pages[render_device::instance().get_frame_resource_index()];

    for (std::size_t index : active_staging_pages)
    {
        m_staging_pages[index].offset = 0;
        m_free_staging_pages.push_back(index);
    }

    active_staging_pages.clear();
}
} // namespace violet
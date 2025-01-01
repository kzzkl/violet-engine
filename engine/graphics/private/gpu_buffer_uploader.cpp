#include "gpu_buffer_uploader.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
gpu_buffer_uploader::gpu_buffer_uploader(std::size_t staging_buffer_size)
    : m_staging_buffer_size(staging_buffer_size)
{
    auto& device = render_device::instance();

    rhi_buffer_desc staging_buffer_desc = {
        .data = nullptr,
        .size = staging_buffer_size,
        .flags = RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_HOST_VISIBLE,
    };

    for (std::size_t i = 0; i < device.get_frame_resource_count(); ++i)
    {
        m_staging_buffers.push_back(device.create_buffer(staging_buffer_desc));
    }
}

void gpu_buffer_uploader::upload(
    rhi_buffer* buffer,
    const void* data,
    std::size_t size,
    std::uint32_t offset)
{
    auto& device = render_device::instance();

    rhi_buffer* staging_buffer = m_staging_buffers[device.get_frame_resource_index()].get();

    std::size_t pending_size = size;

    while (pending_size > 0)
    {
        std::size_t upload_size = std::min(m_staging_buffer_size, size);

        if (m_staging_buffer_offset + upload_size > m_staging_buffer_size)
        {
            rhi_command* command = device.allocate_command();
            record(command);
            device.execute_sync(command);
        }

        std::memcpy(
            static_cast<std::uint8_t*>(staging_buffer->get_buffer_pointer()) +
                m_staging_buffer_offset,
            static_cast<const std::uint8_t*>(data) + (size - pending_size),
            upload_size);

        upload_command upload = {
            .src = staging_buffer,
            .src_offset = static_cast<std::uint32_t>(m_staging_buffer_offset),
            .dst = buffer,
            .dst_offset = offset,
            .size = size,
        };
        m_upload_commands.push_back(upload);

        pending_size -= upload_size;
        m_staging_buffer_offset += upload_size;
    }
}

void gpu_buffer_uploader::record(rhi_command* command)
{
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

    m_staging_buffer_offset = 0;

    m_upload_commands.clear();
}
} // namespace violet
#pragma once

#include "common/allocator.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
class gpu_buffer_uploader
{
public:
    gpu_buffer_uploader(std::size_t staging_buffer_size = 64 * 1024);

    void upload(rhi_buffer* buffer, const void* data, std::size_t size, std::uint32_t offset);
    void record(rhi_command* command);

    bool empty() const noexcept
    {
        return m_upload_commands.empty();
    }

private:
    std::vector<rhi_ptr<rhi_buffer>> m_staging_buffers;
    std::size_t m_staging_buffer_offset{0};
    std::size_t m_staging_buffer_size{0};

    struct upload_command
    {
        rhi_buffer* src;
        std::uint32_t src_offset;
        rhi_buffer* dst;
        std::uint32_t dst_offset;
        std::size_t size;
    };
    std::vector<upload_command> m_upload_commands;
};
} // namespace violet
#pragma once

#include "graphics/render_device.hpp"
#include <unordered_map>

namespace violet
{
class gpu_buffer_uploader
{
public:
    gpu_buffer_uploader(
        std::size_t staging_page_size = 64ull * 1024 * 10,
        std::size_t staging_page_count = 16);

    void tick();

    void upload(
        rhi_buffer* buffer,
        const void* data,
        std::size_t size,
        std::size_t offset,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access);
    void record(rhi_command* command);

    bool empty() const noexcept
    {
        return m_upload_commands.empty();
    }

private:
    struct staging_page
    {
        rhi_ptr<rhi_buffer> buffer;
        std::size_t offset{0};

        std::size_t get_reserve_size() const noexcept
        {
            return buffer->get_size() - offset;
        }

        void copy(const void* data, std::size_t size)
        {
            assert(get_reserve_size() >= size);

            std::memcpy(
                static_cast<std::uint8_t*>(buffer->get_buffer_pointer()) + offset,
                data,
                size);

            offset += size;
        }
    };

    staging_page& allocate_staging_page();

    void flush();
    void reset_active_staging_pages(std::uint32_t frame_resource_index);

    std::vector<std::vector<std::size_t>> m_active_staging_pages;
    std::vector<std::size_t> m_free_staging_pages;
    std::size_t m_current_staging_page_index;

    std::vector<staging_page> m_staging_pages;
    const std::size_t m_staging_page_size;
    const std::size_t m_staging_page_count;

    struct upload_command
    {
        rhi_buffer* src;
        std::uint32_t src_offset;
        rhi_buffer* dst;
        std::uint32_t dst_offset;
        std::size_t size;
    };
    std::vector<upload_command> m_upload_commands;

    std::unordered_map<rhi_buffer*, std::pair<rhi_pipeline_stage_flags, rhi_access_flags>>
        m_dst_buffers;
};
} // namespace violet
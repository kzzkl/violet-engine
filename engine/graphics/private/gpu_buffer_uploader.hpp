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
        const rhi_buffer_region& region,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access);

    void upload(
        rhi_texture* texture,
        const void* data,
        std::size_t size,
        const rhi_texture_region& region,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access,
        rhi_texture_layout layout);

    void record(rhi_command* command);

    bool empty() const noexcept
    {
        return m_buffer_requests.empty() && m_texture_requests.empty();
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

    struct buffer_upload_request
    {
        rhi_buffer* src;
        rhi_buffer_region src_region;

        rhi_buffer* dst;
        rhi_buffer_region dst_region;
    };

    struct buffer_sync_state
    {
        rhi_pipeline_stage_flags stages;
        rhi_access_flags access;
    };

    struct texture_upload_request
    {
        rhi_buffer* src;
        rhi_buffer_region src_region;

        rhi_texture* dst;
        rhi_texture_region dst_region;

        rhi_texture_layout layout;
    };

    struct texture_sync_state
    {
        rhi_pipeline_stage_flags stages;
        rhi_access_flags access;
        rhi_texture_layout layout;
    };

    staging_page& allocate_staging_page(std::size_t expected_size = 0);

    void flush();
    void reset_active_staging_pages(std::uint32_t frame_resource_index);

    void record_buffer_request(rhi_command* command);
    void record_texture_request(rhi_command* command);

    std::vector<std::vector<std::size_t>> m_active_staging_pages;
    std::vector<std::size_t> m_free_staging_pages;
    std::size_t m_current_staging_page_index;

    std::vector<staging_page> m_staging_pages;
    const std::size_t m_staging_page_size;
    const std::size_t m_staging_page_count;

    std::vector<buffer_upload_request> m_buffer_requests;
    std::unordered_map<rhi_buffer*, buffer_sync_state> m_dst_buffers;

    std::vector<texture_upload_request> m_texture_requests;
    std::unordered_map<rhi_texture*, texture_sync_state> m_dst_textures;
};
} // namespace violet
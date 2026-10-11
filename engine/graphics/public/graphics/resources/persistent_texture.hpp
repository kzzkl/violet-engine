#pragma once

#include "graphics/resources/texture.hpp"

namespace violet
{
class gpu_buffer_uploader;
class persistent_texture : public raw_texture
{
public:
    persistent_texture(const rhi_extent& initial_extent, rhi_format format, rhi_buffer_flags flags);

    void copy(const void* data, std::size_t size, rhi_texture_region region);

    void upload(
        gpu_buffer_uploader* uploader,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access,
        rhi_texture_layout layout);

private:
    void reserve(
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access,
        rhi_texture_layout layout);

    struct copy_command
    {
        const void* data;
        std::size_t size;
        rhi_texture_region region;
    };
    std::vector<copy_command> m_copy_queue;

    rhi_extent m_requested_extent;
};
} // namespace violet
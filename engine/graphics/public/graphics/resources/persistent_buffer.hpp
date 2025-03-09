#pragma once

#include "common/allocator.hpp"
#include "graphics/resources/buffer.hpp"

namespace violet
{
class gpu_buffer_uploader;
class persistent_buffer : public raw_buffer
{
public:
    persistent_buffer(std::size_t initial_size, rhi_buffer_flags flags);

    buffer_allocation allocate(std::size_t size);
    void free(buffer_allocation allocation);

    void copy(const void* data, std::size_t size, std::size_t offset);
    void upload(
        gpu_buffer_uploader* uploader,
        rhi_pipeline_stage_flags stages,
        rhi_access_flags access);

private:
    void reserve();

    std::size_t m_allocated_size{0};

    struct copy_command
    {
        const void* data;
        std::size_t size;
        std::size_t offset;
    };
    std::vector<copy_command> m_copy_queue;

    buffer_allocator m_allocator;
};
} // namespace violet
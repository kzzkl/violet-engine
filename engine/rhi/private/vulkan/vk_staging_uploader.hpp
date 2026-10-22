#pragma once

#include "vk_common.hpp"
#include <mutex>

namespace violet::vk
{
class vk_context;

class vk_staging_uploader
{
public:
    vk_staging_uploader(std::size_t staging_buffer_size, vk_context* context);
    ~vk_staging_uploader();

    void upload(const void* data, std::size_t size, VkBuffer buffer, std::size_t offset = 0);

private:
    VkBuffer m_staging_buffer;
    VmaAllocation m_staging_allocation;

    void* m_mapped_data;
    std::size_t m_staging_buffer_size;

    vk_context* m_context;

    std::mutex m_mutex;
};
} // namespace violet::vk
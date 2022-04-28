#pragma once

#include "vk_common.hpp"
#include "vk_pipeline.hpp"
#include <vector>

namespace ash::graphics::vk
{
class vk_command_queue
{
public:
    vk_command_queue(const VkQueue& queue);
    ~vk_command_queue();

    void record(vk_pipeline& pipeline, std::uint32_t image_index);

    VkQueue queue() const noexcept { return m_queue; }

private:
    VkQueue m_queue;
    VkCommandPool m_command_pool;

    std::vector<VkCommandBuffer> m_command_buffers;
};
} // namespace ash::graphics::vk
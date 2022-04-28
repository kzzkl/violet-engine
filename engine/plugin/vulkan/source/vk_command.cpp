#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_renderer.hpp"

namespace ash::graphics::vk
{
vk_command_queue::vk_command_queue(const VkQueue& queue) : m_queue(queue)
{
    auto device = vk_context::device();

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = vk_context::queue_index().graphics;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    throw_if_failed(vkCreateCommandPool(device, &pool_info, nullptr, &m_command_pool));

    std::uint32_t image_count =
        static_cast<std::uint32_t>(vk_context::swap_chain().image_views().size());
    m_command_buffers.resize(image_count);

    VkCommandBufferAllocateInfo allocate_info = {};
    allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocate_info.commandPool = m_command_pool;
    allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocate_info.commandBufferCount = image_count;

    throw_if_failed(vkAllocateCommandBuffers(device, &allocate_info, m_command_buffers.data()));
}

vk_command_queue::~vk_command_queue()
{
    auto device = vk_context::device();

    vkDestroyCommandPool(device, m_command_pool, nullptr);
}

void vk_command_queue::record(vk_pipeline& pipeline, std::uint32_t image_index)
{
    auto& command_buffer = m_command_buffers[image_index];
    throw_if_failed(vkResetCommandBuffer(command_buffer, /*VkCommandBufferResetFlagBits*/ 0));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    throw_if_failed(vkBeginCommandBuffer(command_buffer, &begin_info));

    VkRenderPassBeginInfo pass_info{};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_info.renderPass = pipeline.pass();
    pass_info.framebuffer = pipeline.frame_buffer(image_index);
    pass_info.renderArea.offset = {0, 0};
    pass_info.renderArea.extent = vk_context::swap_chain().extent();

    VkClearValue clear_color = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
    pass_info.clearValueCount = 1;
    pass_info.pClearValues = &clear_color;

    vkCmdBeginRenderPass(command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline());
    vkCmdDraw(command_buffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(command_buffer);

    throw_if_failed(vkEndCommandBuffer(command_buffer));

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {vk_context::image_available_semaphore()};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &command_buffer;

    VkSemaphore signalSemaphores[] = {vk_context::render_finished_semaphore()};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(m_queue, 1, &submitInfo, vk_context::fence()) != VK_SUCCESS)
    {
        throw std::runtime_error("failed to submit draw command buffer!");
    }
}
} // namespace ash::graphics::vk
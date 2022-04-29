#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_frame_buffer.hpp"
#include "vk_render_pass.hpp"

namespace ash::graphics::vk
{
vk_command::vk_command(VkCommandBuffer command_buffer) : m_command_buffer(command_buffer)
{
}

void vk_command::begin(render_pass* pass, frame_buffer* frame_buffer)
{
    auto fb = static_cast<vk_frame_buffer*>(frame_buffer);
    auto rp = static_cast<vk_render_pass*>(pass);
    rp->begin(m_command_buffer, fb->frame_buffer());
}

void vk_command::end(render_pass* pass)
{
    auto rp = static_cast<vk_render_pass*>(pass);
    rp->end(m_command_buffer);
}

void vk_command::begin(render_pipeline* subpass)
{
    auto sp = static_cast<vk_render_pipeline*>(subpass);
    sp->begin(m_command_buffer);
}

void vk_command::end(render_pipeline* subpass)
{
    auto sp = static_cast<vk_render_pipeline*>(subpass);
    sp->end(m_command_buffer);
}

void vk_command::reset()
{
    throw_if_failed(vkResetCommandBuffer(m_command_buffer, 0));

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    throw_if_failed(vkBeginCommandBuffer(m_command_buffer, &begin_info));
}

vk_command_queue::vk_command_queue(const VkQueue& queue, std::size_t frame_resource_count)
    : m_queue(queue),
      m_command_counter(0)
{
    auto device = vk_context::device();

    VkCommandPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_info.queueFamilyIndex = vk_context::queue_index().graphics;
    pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    throw_if_failed(vkCreateCommandPool(device, &pool_info, nullptr, &m_command_pool));

    for (std::size_t i = 0; i < frame_resource_count; ++i)
    {
        std::vector<VkCommandBuffer> command_buffers(4);
        VkCommandBufferAllocateInfo allocate_info = {};
        allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocate_info.commandPool = m_command_pool;
        allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocate_info.commandBufferCount = static_cast<std::uint32_t>(command_buffers.size());
        throw_if_failed(vkAllocateCommandBuffers(device, &allocate_info, command_buffers.data()));

        std::vector<vk_command> commands;
        for (auto command_buffer : command_buffers)
            commands.emplace_back(command_buffer);

        m_commands.push_back(commands);
    }
}

vk_command_queue::~vk_command_queue()
{
    auto device = vk_context::device();
    vkDestroyCommandPool(device, m_command_pool, nullptr);
}

vk_command* vk_command_queue::allocate_command()
{
    auto& command = m_commands[vk_frame_counter::frame_resource_index()][m_command_counter];
    ++m_command_counter;
    command.reset();

    return &command;
}

void vk_command_queue::execute(vk_command* command)
{
    auto command_buffer = command->command_buffer();
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

void vk_command_queue::switch_frame_resources()
{
    m_command_counter = 0;
}
} // namespace ash::graphics::vk
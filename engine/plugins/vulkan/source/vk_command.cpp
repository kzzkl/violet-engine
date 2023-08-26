#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_rhi.hpp"

namespace violet::vk
{
vk_command::vk_command(VkCommandBuffer command_buffer, vk_rhi* rhi) noexcept
    : m_command_buffer(command_buffer),
      m_current_render_pass(nullptr),
      m_rhi(rhi)
{
}

vk_command::~vk_command()
{
}

void vk_command::begin(rhi_render_pass* render_pass, rhi_framebuffer* framebuffer)
{
    VIOLET_VK_ASSERT(m_current_render_pass == nullptr);

    m_current_render_pass = static_cast<vk_render_pass*>(render_pass)->get_render_pass();

    vk_framebuffer* casted_framebuffer = static_cast<vk_framebuffer*>(framebuffer);
    rhi_resource_extent extent = casted_framebuffer->get_extent();
    VkClearValue clear_values = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = m_current_render_pass;
    render_pass_begin_info.framebuffer = casted_framebuffer->get_framebuffer();
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = {extent.width, extent.height};
    render_pass_begin_info.pClearValues = &clear_values;
    render_pass_begin_info.clearValueCount = 1;

    vkCmdBeginRenderPass(m_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport = {};
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
    viewport.x = 0;
    viewport.y = 0;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(m_command_buffer, 0, 1, &viewport);
}

void vk_command::end()
{
    vkCmdEndRenderPass(m_command_buffer);
    m_current_render_pass = nullptr;
}

void vk_command::next()
{
    vkCmdNextSubpass(m_command_buffer, VK_SUBPASS_CONTENTS_INLINE);
}

void vk_command::set_pipeline(rhi_render_pipeline* render_pipeline)
{
    vk_render_pipeline* casted_render_pipeline = static_cast<vk_render_pipeline*>(render_pipeline);
    vkCmdBindPipeline(
        m_command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        casted_render_pipeline->get_pipeline());
}

void vk_command::set_parameter(std::size_t index, rhi_pipeline_parameter* parameter)
{
}

void vk_command::set_viewport(const rhi_viewport& viewport)
{
    VkViewport vp = {
        .x = viewport.x,
        .y = viewport.y,
        .width = viewport.width,
        .height = viewport.height,
        .minDepth = viewport.min_depth,
        .maxDepth = viewport.max_depth};
    vkCmdSetViewport(m_command_buffer, 0, 1, &vp);
}

void vk_command::set_scissor(const rhi_scissor_rect* rects, std::size_t size)
{
    std::vector<VkRect2D> scissors;
    for (std::size_t i = 0; i < size; ++i)
    {
        VkRect2D rect = {};
        rect.offset.x = rects[i].min_x;
        rect.offset.y = rects[i].min_y;
        rect.extent.width = rects[i].max_x - rects[i].min_x;
        rect.extent.height = rects[i].max_y - rects[i].min_y;
        scissors.push_back(rect);
    }
    vkCmdSetScissor(
        m_command_buffer,
        0,
        static_cast<std::uint32_t>(scissors.size()),
        scissors.data());
}

void vk_command::set_vertex_buffers(
    rhi_resource* const* vertex_buffers,
    std::size_t vertex_buffer_count)
{
    std::vector<VkBuffer> buffers(vertex_buffer_count);
    std::vector<VkDeviceSize> offsets(vertex_buffer_count);
    for (std::size_t i = 0; i < vertex_buffer_count; ++i)
    {
        buffers[i] = static_cast<vk_vertex_buffer*>(vertex_buffers[i])->get_buffer_handle();
        offsets[i] = 0;
    }
    vkCmdBindVertexBuffers(
        m_command_buffer,
        0,
        static_cast<std::uint32_t>(vertex_buffer_count),
        buffers.data(),
        offsets.data());
}

void vk_command::set_index_buffer(rhi_resource* index_buffer)
{
    vk_index_buffer* buffer = static_cast<vk_index_buffer*>(index_buffer);
    vkCmdBindIndexBuffer(
        m_command_buffer,
        buffer->get_buffer_handle(),
        0,
        buffer->get_index_type());
}

void vk_command::draw(std::size_t vertex_start, std::size_t vertex_count)
{
    vkCmdDraw(m_command_buffer, vertex_count, 1, vertex_start, 0);
}

void vk_command::draw_indexed(
    std::size_t index_start,
    std::size_t index_count,
    std::size_t vertex_base)
{
    vkCmdDrawIndexed(m_command_buffer, index_count, 1, 0, 0, 0);
}

void vk_command::clear_render_target(rhi_resource* render_target, const float4& color)
{
}

void vk_command::clear_depth_stencil(
    rhi_resource* depth_stencil,
    bool clear_depth,
    float depth,
    bool clear_stencil,
    std::uint8_t stencil)
{
}

void vk_command::reset()
{
    m_current_render_pass = nullptr;
    throw_if_failed(vkResetCommandBuffer(m_command_buffer, 0));
}

vk_command_queue::vk_command_queue(std::uint32_t queue_family_index, vk_rhi* rhi) : m_rhi(rhi)
{
    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_index;

    throw_if_failed(
        vkCreateCommandPool(m_rhi->get_device(), &command_pool_info, nullptr, &m_command_pool));

    m_active_commands.resize(m_rhi->get_frame_resource_count());

    vkGetDeviceQueue(m_rhi->get_device(), queue_family_index, 0, &m_queue);

    m_fence = std::make_unique<vk_fence>(false, m_rhi);
}

vk_command_queue::~vk_command_queue()
{
    vkDestroyCommandPool(m_rhi->get_device(), m_command_pool, nullptr);
}

vk_command* vk_command_queue::allocate_command()
{
    if (m_free_commands.empty())
    {
        VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
        command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        command_buffer_allocate_info.commandPool = m_command_pool;
        command_buffer_allocate_info.commandBufferCount = 1;

        std::vector<VkCommandBuffer> command_buffers(
            command_buffer_allocate_info.commandBufferCount);
        throw_if_failed(vkAllocateCommandBuffers(
            m_rhi->get_device(),
            &command_buffer_allocate_info,
            command_buffers.data()));

        for (VkCommandBuffer command_buffer : command_buffers)
        {
            std::unique_ptr<vk_command> command =
                std::make_unique<vk_command>(command_buffer, m_rhi);
            m_free_commands.push_back(command.get());
            m_commands.push_back(std::move(command));
        }
    }

    vk_command* command = m_free_commands.back();
    m_free_commands.pop_back();
    m_active_commands[m_rhi->get_frame_resource_index()].push_back(command);

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    throw_if_failed(
        vkBeginCommandBuffer(command->get_command_buffer(), &command_buffer_begin_info));

    return command;
}

void vk_command_queue::execute(
    rhi_render_command* const* commands,
    std::size_t command_count,
    rhi_semaphore* const* signal_semaphores,
    std::size_t signal_semaphore_count,
    rhi_semaphore* const* wait_semaphores,
    std::size_t wait_semaphore_count,
    rhi_fence* fence)
{
    std::vector<VkCommandBuffer> vk_commands(command_count);
    for (std::size_t i = 0; i < command_count; ++i)
    {
        vk_commands[i] = static_cast<vk_command*>(commands[i])->get_command_buffer();
        throw_if_failed(vkEndCommandBuffer(vk_commands[i]));
    }

    std::vector<VkSemaphore> vk_signal_semaphores(signal_semaphore_count);
    for (std::size_t i = 0; i < signal_semaphore_count; ++i)
        vk_signal_semaphores[i] = static_cast<vk_semaphore*>(signal_semaphores[i])->get_semaphore();

    std::vector<VkSemaphore> vk_wait_semaphores(wait_semaphore_count);
    for (std::size_t i = 0; i < wait_semaphore_count; ++i)
        vk_wait_semaphores[i] = static_cast<vk_semaphore*>(wait_semaphores[i])->get_semaphore();

    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pSignalSemaphores = vk_signal_semaphores.data();
    submit_info.signalSemaphoreCount = static_cast<std::uint32_t>(vk_signal_semaphores.size());
    submit_info.pWaitSemaphores = vk_wait_semaphores.data();
    submit_info.waitSemaphoreCount = static_cast<std::uint32_t>(vk_wait_semaphores.size());
    submit_info.pWaitDstStageMask = wait_stages;
    submit_info.commandBufferCount = static_cast<std::uint32_t>(vk_commands.size());
    submit_info.pCommandBuffers = vk_commands.data();

    throw_if_failed(vkQueueSubmit(
        m_queue,
        1,
        &submit_info,
        fence == nullptr ? VK_NULL_HANDLE : static_cast<vk_fence*>(fence)->get_fence()));
}

void vk_command_queue::execute_sync(rhi_render_command* command)
{
    execute(&command, 1, nullptr, 0, nullptr, 0, m_fence.get());
    m_fence->wait();
}

void vk_command_queue::begin_frame()
{
    auto& commands = m_active_commands[m_rhi->get_frame_resource_index()];
    for (vk_command* command : commands)
    {
        command->reset();
        m_free_commands.push_back(command);
    }
    commands.clear();
}
} // namespace violet::vk
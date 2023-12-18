#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_resource.hpp"
#include "vk_util.hpp"

namespace violet::vk
{
vk_command::vk_command(VkCommandBuffer command_buffer, vk_context* context) noexcept
    : m_command_buffer(command_buffer),
      m_current_render_pass(VK_NULL_HANDLE),
      m_current_pipeline_layout(VK_NULL_HANDLE),
      m_context(context)
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
    auto& clear_values = casted_framebuffer->get_clear_values();

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = m_current_render_pass;
    render_pass_begin_info.framebuffer = casted_framebuffer->get_framebuffer();
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = {extent.width, extent.height};
    render_pass_begin_info.pClearValues = clear_values.data();
    render_pass_begin_info.clearValueCount = static_cast<std::uint32_t>(clear_values.size());

    vkCmdBeginRenderPass(m_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
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

void vk_command::set_render_pipeline(rhi_render_pipeline* render_pipeline)
{
    vk_render_pipeline* pipeline = static_cast<vk_render_pipeline*>(render_pipeline);
    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
}

void vk_command::set_render_parameter(std::size_t index, rhi_parameter* parameter)
{
    vk_parameter* p = static_cast<vk_parameter*>(parameter);
    p->sync();

    VkDescriptorSet descriptor_sets[] = {p->get_descriptor_set()};
    vkCmdBindDescriptorSets(
        m_command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_current_pipeline_layout,
        static_cast<std::uint32_t>(index),
        1,
        descriptor_sets,
        0,
        nullptr);
}

void vk_command::set_compute_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    vk_compute_pipeline* pipeline = static_cast<vk_compute_pipeline*>(compute_pipeline);
    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
}

void vk_command::set_compute_parameter(std::size_t index, rhi_parameter* parameter)
{
    vk_parameter* p = static_cast<vk_parameter*>(parameter);
    p->sync();

    VkDescriptorSet descriptor_sets[] = {p->get_descriptor_set()};
    vkCmdBindDescriptorSets(
        m_command_buffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        m_current_pipeline_layout,
        static_cast<std::uint32_t>(index),
        1,
        descriptor_sets,
        0,
        nullptr);
}

void vk_command::set_viewport(const rhi_viewport& viewport)
{
    VkViewport vp = {
        .x = viewport.x,
        .y = viewport.height - viewport.y,
        .width = viewport.width,
        .height = -viewport.height,
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
    vkCmdDrawIndexed(m_command_buffer, index_count, 1, index_start, vertex_base, 0);
}

void vk_command::dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    vkCmdDispatch(m_command_buffer, x, y, z);
}

void vk_command::set_pipeline_barrier(
    rhi_pipeline_stage_flags src_state,
    rhi_pipeline_stage_flags dst_state,
    const rhi_buffer_barrier* const buffer_barriers,
    std::size_t buffer_barrier_count,
    const rhi_texture_barrier* const texture_barriers,
    std::size_t texture_barrier_count)
{
    std::vector<VkBufferMemoryBarrier> vk_buffer_barriers(buffer_barrier_count);
    for (std::size_t i = 0; i < buffer_barrier_count; ++i)
    {
        VkBufferMemoryBarrier& barrier = vk_buffer_barriers[i];
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    }

    std::vector<VkImageMemoryBarrier> vk_image_barriers(texture_barrier_count);
    for (std::size_t i = 0; i < texture_barrier_count; ++i)
    {
        VkImageMemoryBarrier& barrier = vk_image_barriers[i];
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = vk_util::map_access_flags(texture_barriers[i].src_access);
        barrier.dstAccessMask = vk_util::map_access_flags(texture_barriers[i].dst_access);
        barrier.oldLayout = vk_util::map_state(texture_barriers[i].src_state);
        barrier.newLayout = vk_util::map_state(texture_barriers[i].dst_state);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = static_cast<vk_image*>(texture_barriers[i].texture)->get_image();
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
    }

    vkCmdPipelineBarrier(
        m_command_buffer,
        vk_util::map_pipeline_stage_flags(src_state),
        vk_util::map_pipeline_stage_flags(dst_state),
        0,
        0,
        nullptr,
        vk_buffer_barriers.size(),
        vk_buffer_barriers.data(),
        vk_image_barriers.size(),
        vk_image_barriers.data());
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
    m_current_render_pass = VK_NULL_HANDLE;
    m_current_pipeline_layout = VK_NULL_HANDLE;
    vk_check(vkResetCommandBuffer(m_command_buffer, 0));
}

vk_graphics_queue::vk_graphics_queue(std::uint32_t queue_family_index, vk_context* context)
    : m_family_index(queue_family_index),
      m_context(context)
{
    VkCommandPoolCreateInfo command_pool_info = {};
    command_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_info.queueFamilyIndex = queue_family_index;

    vk_check(
        vkCreateCommandPool(m_context->get_device(), &command_pool_info, nullptr, &m_command_pool));

    m_active_commands.resize(m_context->get_frame_resource_count());

    vkGetDeviceQueue(m_context->get_device(), queue_family_index, 0, &m_queue);

    m_fence = std::make_unique<vk_fence>(false, m_context);
}

vk_graphics_queue::~vk_graphics_queue()
{
    vkDestroyCommandPool(m_context->get_device(), m_command_pool, nullptr);
}

vk_command* vk_graphics_queue::allocate_command()
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
        vk_check(vkAllocateCommandBuffers(
            m_context->get_device(),
            &command_buffer_allocate_info,
            command_buffers.data()));

        for (VkCommandBuffer command_buffer : command_buffers)
        {
            std::unique_ptr<vk_command> command =
                std::make_unique<vk_command>(command_buffer, m_context);
            m_free_commands.push_back(command.get());
            m_commands.push_back(std::move(command));
        }
    }

    vk_command* command = m_free_commands.back();
    m_free_commands.pop_back();
    m_active_commands[m_context->get_frame_resource_index()].push_back(command);

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vk_check(vkBeginCommandBuffer(command->get_command_buffer(), &command_buffer_begin_info));

    return command;
}

void vk_graphics_queue::execute(
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
        vk_check(vkEndCommandBuffer(vk_commands[i]));
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

    vk_check(vkQueueSubmit(
        m_queue,
        1,
        &submit_info,
        fence == nullptr ? VK_NULL_HANDLE : static_cast<vk_fence*>(fence)->get_fence()));
}

void vk_graphics_queue::execute_sync(rhi_render_command* command)
{
    execute(&command, 1, nullptr, 0, nullptr, 0, m_fence.get());
    m_fence->wait();
}

void vk_graphics_queue::begin_frame()
{
    auto& commands = m_active_commands[m_context->get_frame_resource_index()];
    for (vk_command* command : commands)
    {
        command->reset();
        m_free_commands.push_back(command);
    }
    commands.clear();
}

vk_present_queue::vk_present_queue(std::uint32_t queue_family_index, vk_context* context)
    : m_family_index(queue_family_index)
{
    vkGetDeviceQueue(context->get_device(), queue_family_index, 0, &m_queue);
}

vk_present_queue::~vk_present_queue()
{
}

void vk_present_queue::present(
    VkSwapchainKHR swapchain,
    std::uint32_t image_index,
    rhi_semaphore* const* wait_semaphores,
    std::size_t wait_semaphore_count)
{
    VkSwapchainKHR swapchains[] = {swapchain};
    std::uint32_t image_indices[] = {image_index};

    std::vector<VkSemaphore> semaphores(wait_semaphore_count);
    for (std::size_t i = 0; i < wait_semaphore_count; ++i)
        semaphores[i] = static_cast<vk_semaphore*>(wait_semaphores[i])->get_semaphore();

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pWaitSemaphores = semaphores.data();
    present_info.waitSemaphoreCount = static_cast<std::uint32_t>(semaphores.size());
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = image_indices;

    VkResult result = vkQueuePresentKHR(m_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        return;
    else
        vk_check(result);
}
} // namespace violet::vk
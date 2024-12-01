#include "vk_command.hpp"
#include "vk_framebuffer.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_resource.hpp"
#include "vk_util.hpp"
#include <cassert>

namespace violet::vk
{
vk_command::vk_command(VkCommandBuffer command_buffer, vk_context* context) noexcept
    : m_command_buffer(command_buffer),
      m_current_render_pass(VK_NULL_HANDLE),
      m_current_pipeline_layout(VK_NULL_HANDLE),
      m_context(context)
{
}

vk_command::~vk_command() {}

void vk_command::begin_render_pass(rhi_render_pass* render_pass, rhi_framebuffer* framebuffer)
{
    assert(m_current_render_pass == VK_NULL_HANDLE);

    m_current_render_pass = static_cast<vk_render_pass*>(render_pass)->get_render_pass();

    vk_framebuffer* casted_framebuffer = static_cast<vk_framebuffer*>(framebuffer);
    rhi_texture_extent extent = casted_framebuffer->get_extent();
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

void vk_command::end_render_pass()
{
    vkCmdEndRenderPass(m_command_buffer);
    m_current_render_pass = VK_NULL_HANDLE;
}

void vk_command::next_subpass()
{
    vkCmdNextSubpass(m_command_buffer, VK_SUBPASS_CONTENTS_INLINE);
}

void vk_command::set_pipeline(rhi_render_pipeline* render_pipeline)
{
    vk_render_pipeline* pipeline = static_cast<vk_render_pipeline*>(render_pipeline);
    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
    m_current_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void vk_command::set_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    vk_compute_pipeline* pipeline = static_cast<vk_compute_pipeline*>(compute_pipeline);
    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
    m_current_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
}

void vk_command::set_parameter(std::size_t index, rhi_parameter* parameter)
{
    VkDescriptorSet descriptor_set = static_cast<vk_parameter*>(parameter)->get_descriptor_set();
    vkCmdBindDescriptorSets(
        m_command_buffer,
        m_current_bind_point,
        m_current_pipeline_layout,
        static_cast<std::uint32_t>(index),
        1,
        &descriptor_set,
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
    rhi_buffer* const* vertex_buffers,
    std::size_t vertex_buffer_count)
{
    std::vector<VkBuffer> buffers(vertex_buffer_count);
    std::vector<VkDeviceSize> offsets(vertex_buffer_count);
    for (std::size_t i = 0; i < vertex_buffer_count; ++i)
    {
        buffers[i] = static_cast<vk_buffer*>(vertex_buffers[i])->get_buffer();
        offsets[i] = 0;
    }
    vkCmdBindVertexBuffers(
        m_command_buffer,
        0,
        static_cast<std::uint32_t>(vertex_buffer_count),
        buffers.data(),
        offsets.data());
}

void vk_command::set_index_buffer(rhi_buffer* index_buffer)
{
    vk_index_buffer* buffer = static_cast<vk_index_buffer*>(index_buffer);
    vkCmdBindIndexBuffer(m_command_buffer, buffer->get_buffer(), 0, buffer->get_index_type());
}

void vk_command::draw(std::size_t vertex_offset, std::size_t vertex_count)
{
    vkCmdDraw(
        m_command_buffer,
        static_cast<std::uint32_t>(vertex_count),
        1,
        static_cast<std::uint32_t>(vertex_offset),
        0);
}

void vk_command::draw_indexed(
    std::size_t index_offset,
    std::size_t index_count,
    std::size_t vertex_base)
{
    vkCmdDrawIndexed(
        m_command_buffer,
        static_cast<std::uint32_t>(index_count),
        1,
        static_cast<std::uint32_t>(index_offset),
        static_cast<std::uint32_t>(vertex_base),
        0);
}

void vk_command::draw_indexed_indirect(
    rhi_buffer* command_buffer,
    std::size_t command_buffer_offset,
    rhi_buffer* count_buffer,
    std::size_t count_buffer_offset,
    std::size_t max_draw_count)
{
    vkCmdDrawIndexedIndirectCount(
        m_command_buffer,
        static_cast<vk_buffer*>(command_buffer)->get_buffer(),
        command_buffer_offset,
        static_cast<vk_buffer*>(count_buffer)->get_buffer(),
        count_buffer_offset,
        max_draw_count,
        sizeof(VkDrawIndexedIndirectCommand));
}

void vk_command::dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z)
{
    vkCmdDispatch(m_command_buffer, x, y, z);
}

void vk_command::set_pipeline_barrier(
    rhi_pipeline_stage_flags src_stages,
    rhi_pipeline_stage_flags dst_stages,
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
        barrier.srcAccessMask = vk_util::map_access_flags(buffer_barriers[i].src_access);
        barrier.dstAccessMask = vk_util::map_access_flags(buffer_barriers[i].dst_access);
        barrier.buffer = static_cast<vk_buffer*>(buffer_barriers[i].buffer)->get_buffer();
        barrier.offset = buffer_barriers[i].offset;
        barrier.size = buffer_barriers[i].size;
    }

    std::vector<VkImageMemoryBarrier> vk_image_barriers(texture_barrier_count);
    for (std::size_t i = 0; i < texture_barrier_count; ++i)
    {
        vk_image* image = static_cast<vk_image*>(texture_barriers[i].texture);

        VkImageMemoryBarrier& barrier = vk_image_barriers[i];
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.srcAccessMask = vk_util::map_access_flags(texture_barriers[i].src_access);
        barrier.dstAccessMask = vk_util::map_access_flags(texture_barriers[i].dst_access);
        barrier.oldLayout = vk_util::map_layout(texture_barriers[i].src_layout);
        barrier.newLayout = vk_util::map_layout(texture_barriers[i].dst_layout);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image->get_image();
        barrier.subresourceRange.aspectMask = image->get_aspect_mask();
        barrier.subresourceRange.baseMipLevel = texture_barriers[i].level;
        barrier.subresourceRange.levelCount = texture_barriers[i].level_count;
        barrier.subresourceRange.baseArrayLayer = texture_barriers[i].layer;
        barrier.subresourceRange.layerCount = texture_barriers[i].layer_count;
    }

    vkCmdPipelineBarrier(
        m_command_buffer,
        vk_util::map_pipeline_stage_flags(src_stages),
        vk_util::map_pipeline_stage_flags(dst_stages),
        0,
        0,
        nullptr,
        vk_buffer_barriers.size(),
        vk_buffer_barriers.data(),
        vk_image_barriers.size(),
        vk_image_barriers.data());
}

void vk_command::copy_texture(
    rhi_texture* src,
    const rhi_texture_region& src_region,
    rhi_texture* dst,
    const rhi_texture_region& dst_region)
{
    assert(
        src_region.extent.width == dst_region.extent.width &&
        src_region.extent.height == dst_region.extent.height);

    vk_image* src_image = static_cast<vk_image*>(src);
    vk_image* dst_image = static_cast<vk_image*>(dst);

    VkImageCopy image_copy = {};
    image_copy.extent = {src_region.extent.width, src_region.extent.height, 1};

    image_copy.srcOffset = {src_region.offset_x, src_region.offset_y, 0};
    image_copy.srcSubresource.aspectMask = src_image->get_aspect_mask();
    image_copy.srcSubresource.mipLevel = src_region.level;
    image_copy.srcSubresource.baseArrayLayer = src_region.layer;
    image_copy.srcSubresource.layerCount = src_region.layer_count;

    image_copy.dstOffset = {dst_region.offset_x, dst_region.offset_y, 0};
    image_copy.dstSubresource.aspectMask = dst_image->get_aspect_mask();
    image_copy.dstSubresource.mipLevel = dst_region.level;
    image_copy.dstSubresource.baseArrayLayer = dst_region.layer;
    image_copy.dstSubresource.layerCount = dst_region.layer_count;

    vkCmdCopyImage(
        m_command_buffer,
        src_image->get_image(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst_image->get_image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &image_copy);
}

void vk_command::blit_texture(
    rhi_texture* src,
    const rhi_texture_region& src_region,
    rhi_texture* dst,
    const rhi_texture_region& dst_region)
{
    vk_image* src_image = static_cast<vk_image*>(src);
    vk_image* dst_image = static_cast<vk_image*>(dst);

    VkImageBlit image_blit = {};

    image_blit.srcOffsets[0] = {src_region.offset_x, src_region.offset_y, 0};
    image_blit.srcOffsets[1] = {
        static_cast<std::int32_t>(src_region.extent.width),
        static_cast<std::int32_t>(src_region.extent.height),
        1};
    image_blit.srcSubresource.aspectMask = src_image->get_aspect_mask();
    image_blit.srcSubresource.mipLevel = src_region.level;
    image_blit.srcSubresource.baseArrayLayer = src_region.layer;
    image_blit.srcSubresource.layerCount = src_region.layer_count;

    image_blit.dstOffsets[0] = {dst_region.offset_x, dst_region.offset_y, 0};
    image_blit.dstOffsets[1] = {
        static_cast<std::int32_t>(dst_region.extent.width),
        static_cast<std::int32_t>(dst_region.extent.height),
        1};
    image_blit.dstSubresource.aspectMask = dst_image->get_aspect_mask();
    image_blit.dstSubresource.mipLevel = dst_region.level;
    image_blit.dstSubresource.baseArrayLayer = dst_region.layer;
    image_blit.dstSubresource.layerCount = dst_region.layer_count;

    vkCmdBlitImage(
        m_command_buffer,
        src_image->get_image(),
        VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        dst_image->get_image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &image_blit,
        VK_FILTER_LINEAR);
}

void vk_command::fill_buffer(
    rhi_buffer* buffer,
    const rhi_buffer_region& region,
    std::uint32_t value)
{
    assert(region.size > 0 && region.size % 4 == 0);

    vkCmdFillBuffer(
        m_command_buffer,
        static_cast<vk_buffer*>(buffer)->get_buffer(),
        region.offset,
        region.size,
        value);
}

void vk_command::copy_buffer(
    rhi_buffer* src,
    const rhi_buffer_region& src_region,
    rhi_buffer* dst,
    const rhi_buffer_region& dst_region)
{
    assert(src_region.size == dst_region.size);

    VkBuffer src_buffer = static_cast<vk_buffer*>(src)->get_buffer();
    VkBuffer dst_buffer = static_cast<vk_buffer*>(dst)->get_buffer();

    VkBufferCopy buffer_copy = {};
    buffer_copy.srcOffset = src_region.offset;
    buffer_copy.dstOffset = dst_region.offset;
    buffer_copy.size = src_region.size;

    vkCmdCopyBuffer(m_command_buffer, src_buffer, dst_buffer, 1, &buffer_copy);
}

void vk_command::copy_buffer_to_texture(
    rhi_buffer* buffer,
    const rhi_buffer_region& buffer_region,
    rhi_texture* texture,
    const rhi_texture_region& texture_region)
{
    vk_buffer* src_buffer = static_cast<vk_buffer*>(buffer);
    vk_image* dst_image = static_cast<vk_image*>(texture);

    VkBufferMemoryBarrier buffer_barrier = {};
    buffer_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    buffer_barrier.srcAccessMask = VK_ACCESS_NONE;
    buffer_barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    buffer_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buffer_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    buffer_barrier.buffer = src_buffer->get_buffer();
    buffer_barrier.offset = buffer_region.offset;
    buffer_barrier.size = buffer_region.size;

    VkImageMemoryBarrier texture_barrier = {};
    texture_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    texture_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    texture_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    texture_barrier.image = dst_image->get_image();
    texture_barrier.subresourceRange.baseMipLevel = texture_region.level;
    texture_barrier.subresourceRange.levelCount = 1;
    texture_barrier.subresourceRange.baseArrayLayer = texture_region.layer;
    texture_barrier.subresourceRange.layerCount = texture_region.layer_count;
    texture_barrier.subresourceRange.aspectMask = dst_image->get_aspect_mask();
    texture_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    texture_barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    texture_barrier.srcAccessMask = 0;
    texture_barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    vkCmdPipelineBarrier(
        m_command_buffer,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        1,
        &buffer_barrier,
        1,
        &texture_barrier);

    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = dst_image->get_aspect_mask();
    region.imageSubresource.mipLevel = texture_region.level;
    region.imageSubresource.baseArrayLayer = texture_region.layer;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {texture_region.extent.width, texture_region.extent.height, 1};

    vkCmdCopyBufferToImage(
        m_command_buffer,
        src_buffer->get_buffer(),
        dst_image->get_image(),
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region);
}

void vk_command::signal(rhi_fence* fence, std::uint64_t value)
{
    m_signal_fences.push_back(static_cast<vk_fence*>(fence)->get_semaphore());
    m_signal_values.push_back(value);
}

void vk_command::wait(rhi_fence* fence, std::uint64_t value, rhi_pipeline_stage_flags stages)
{
    m_wait_fences.push_back(static_cast<vk_fence*>(fence)->get_semaphore());
    m_wait_values.push_back(value);
    m_wait_stages.push_back(vk_util::map_pipeline_stage_flags(stages));
}

void vk_command::reset()
{
    m_signal_fences.clear();
    m_signal_values.clear();
    m_wait_fences.clear();
    m_wait_values.clear();
    m_wait_stages.clear();

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

    m_fence = std::make_unique<vk_fence>(true, m_context);
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

void vk_graphics_queue::execute(rhi_command* command)
{
    vk_command* cast_command = static_cast<vk_command*>(command);

    VkCommandBuffer buffer = cast_command->get_command_buffer();
    vk_check(vkEndCommandBuffer(buffer));

    auto& signal_semaphores = cast_command->get_signal_fences();
    auto& signal_values = cast_command->get_signal_values();

    auto& wait_semaphores = cast_command->get_wait_fences();
    auto& wait_values = cast_command->get_wait_values();
    auto& wait_stages = cast_command->get_wait_stages();

    VkTimelineSemaphoreSubmitInfo timeline_info;
    timeline_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
    timeline_info.pNext = NULL;
    timeline_info.signalSemaphoreValueCount = static_cast<std::uint32_t>(signal_values.size());
    timeline_info.pSignalSemaphoreValues = signal_values.data();
    timeline_info.waitSemaphoreValueCount = static_cast<std::uint32_t>(wait_values.size());
    timeline_info.pWaitSemaphoreValues = wait_values.data();

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = &timeline_info;
    submit_info.pSignalSemaphores = signal_semaphores.data();
    submit_info.signalSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size());
    submit_info.pWaitSemaphores = wait_semaphores.data();
    submit_info.waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size());
    submit_info.pWaitDstStageMask = wait_stages.data();
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &buffer;

    vk_check(vkQueueSubmit(m_queue, 1, &submit_info, VK_NULL_HANDLE));
}

void vk_graphics_queue::execute_sync(rhi_command* command)
{
    command->signal(m_fence.get(), ++m_fence_value);
    execute(command);
    m_fence->wait(m_fence_value);
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

vk_present_queue::~vk_present_queue() {}

void vk_present_queue::present(
    VkSwapchainKHR swapchain,
    std::uint32_t image_index,
    VkSemaphore wait_semaphore)
{
    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pWaitSemaphores = &wait_semaphore;
    present_info.waitSemaphoreCount = 1;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &image_index;

    VkResult result = vkQueuePresentKHR(m_queue, &present_info);
    if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR)
    {
        vk_check(result);
    }
}
} // namespace violet::vk
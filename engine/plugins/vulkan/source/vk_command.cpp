#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_layout.hpp"
#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_resource.hpp"
#include "vk_utils.hpp"
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

void vk_command::begin_render_pass(
    rhi_render_pass* render_pass,
    const rhi_attachment* attachments,
    std::uint32_t attachment_count)
{
    assert(m_current_render_pass == nullptr);

    m_current_render_pass = static_cast<vk_render_pass*>(render_pass);
    m_current_render_pass->begin(m_command_buffer, attachments, attachment_count);
}

void vk_command::end_render_pass()
{
    m_current_render_pass->end(m_command_buffer);
    m_current_render_pass = nullptr;
}

void vk_command::set_pipeline(rhi_raster_pipeline* raster_pipeline)
{
    auto* pipeline = static_cast<vk_raster_pipeline*>(raster_pipeline);
    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
    m_current_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void vk_command::set_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    auto* pipeline = static_cast<vk_compute_pipeline*>(compute_pipeline);
    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
    m_current_bind_point = VK_PIPELINE_BIND_POINT_COMPUTE;
}

void vk_command::set_parameter(std::uint32_t index, rhi_parameter* parameter)
{
    VkDescriptorSet descriptor_set = static_cast<vk_parameter*>(parameter)->get_descriptor_set();
    vkCmdBindDescriptorSets(
        m_command_buffer,
        m_current_bind_point,
        m_current_pipeline_layout->get_layout(),
        index,
        1,
        &descriptor_set,
        0,
        nullptr);
}

void vk_command::set_constant(const void* data, std::size_t size)
{
    assert(m_current_pipeline_layout->get_push_constant_size() >= size);

    vkCmdPushConstants(
        m_command_buffer,
        m_current_pipeline_layout->get_layout(),
        m_current_pipeline_layout->get_push_constant_stages(),
        0,
        static_cast<std::uint32_t>(size),
        data);
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

void vk_command::set_scissor(const rhi_scissor_rect* rects, std::uint32_t rect_count)
{
    std::vector<VkRect2D> scissors;
    for (std::uint32_t i = 0; i < rect_count; ++i)
    {
        VkRect2D rect = {
            .offset =
                {
                    .x = static_cast<std::int32_t>(rects[i].min_x),
                    .y = static_cast<std::int32_t>(rects[i].min_y),
                },
            .extent = {
                .width = rects[i].max_x - rects[i].min_x,
                .height = rects[i].max_y - rects[i].min_y,
            }};
        scissors.push_back(rect);
    }
    vkCmdSetScissor(m_command_buffer, 0, rect_count, scissors.data());
}

void vk_command::set_vertex_buffers(
    rhi_buffer* const* vertex_buffers,
    std::uint32_t vertex_buffer_count)
{
    std::vector<VkBuffer> buffers(vertex_buffer_count);
    std::vector<VkDeviceSize> offsets(vertex_buffer_count, 0);
    for (std::size_t i = 0; i < vertex_buffer_count; ++i)
    {
        buffers[i] = vertex_buffers[i] ? static_cast<vk_buffer*>(vertex_buffers[i])->get_buffer() :
                                         VK_NULL_HANDLE;
    }
    vkCmdBindVertexBuffers(
        m_command_buffer,
        0,
        vertex_buffer_count,
        buffers.data(),
        offsets.data());
}

void vk_command::set_index_buffer(rhi_buffer* index_buffer, std::size_t index_size)
{
    auto* buffer = static_cast<vk_buffer*>(index_buffer);

    VkIndexType index_type = VK_INDEX_TYPE_UINT32;
    switch (index_size)
    {
    case 2:
        index_type = VK_INDEX_TYPE_UINT16;
        break;
    case 4:
        index_type = VK_INDEX_TYPE_UINT32;
        break;
    default:
        throw std::runtime_error("unsupported index size");
    };

    vkCmdBindIndexBuffer(m_command_buffer, buffer->get_buffer(), 0, index_type);
}

void vk_command::draw(
    std::uint32_t vertex_offset,
    std::uint32_t vertex_count,
    std::uint32_t instance_offset,
    std::uint32_t instance_count)
{
    vkCmdDraw(m_command_buffer, vertex_count, instance_count, vertex_offset, instance_offset);
}

void vk_command::draw_indexed(
    std::uint32_t index_offset,
    std::uint32_t index_count,
    std::uint32_t vertex_offset,
    std::uint32_t instance_offset,
    std::uint32_t instance_count)
{
    vkCmdDrawIndexed(
        m_command_buffer,
        index_count,
        instance_count,
        index_offset,
        static_cast<std::int32_t>(vertex_offset),
        instance_offset);
}

void vk_command::draw_indexed_indirect(
    rhi_buffer* command_buffer,
    std::size_t command_buffer_offset,
    rhi_buffer* count_buffer,
    std::size_t count_buffer_offset,
    std::uint32_t max_draw_count)
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

void vk_command::dispatch_indirect(rhi_buffer* command_buffer, std::size_t command_buffer_offset)
{
    vkCmdDispatchIndirect(
        m_command_buffer,
        static_cast<vk_buffer*>(command_buffer)->get_buffer(),
        command_buffer_offset);
}

void vk_command::set_pipeline_barrier(
    const rhi_buffer_barrier* const buffer_barriers,
    std::uint32_t buffer_barrier_count,
    const rhi_texture_barrier* const texture_barriers,
    std::uint32_t texture_barrier_count)
{
    assert(buffer_barrier_count != 0 || texture_barrier_count != 0);

    rhi_pipeline_stage_flags src_stages = 0;
    rhi_pipeline_stage_flags dst_stages = 0;

    std::vector<VkBufferMemoryBarrier> vk_buffer_barriers(buffer_barrier_count);
    for (std::size_t i = 0; i < buffer_barrier_count; ++i)
    {
        vk_buffer_barriers[i] = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
            .srcAccessMask = vk_utils::map_access_flags(buffer_barriers[i].src_access),
            .dstAccessMask = vk_utils::map_access_flags(buffer_barriers[i].dst_access),
            .buffer = static_cast<vk_buffer*>(buffer_barriers[i].buffer)->get_buffer(),
            .offset = buffer_barriers[i].offset,
            .size = buffer_barriers[i].size,
        };

        src_stages |= buffer_barriers[i].src_stages;
        dst_stages |= buffer_barriers[i].dst_stages;
    }

    std::vector<VkImageMemoryBarrier> vk_image_barriers(texture_barrier_count);
    for (std::size_t i = 0; i < texture_barrier_count; ++i)
    {
        auto* image = static_cast<vk_texture*>(texture_barriers[i].texture);

        vk_image_barriers[i] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = vk_utils::map_access_flags(texture_barriers[i].src_access),
            .dstAccessMask = vk_utils::map_access_flags(texture_barriers[i].dst_access),
            .oldLayout = vk_utils::map_layout(texture_barriers[i].src_layout),
            .newLayout = vk_utils::map_layout(texture_barriers[i].dst_layout),
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = image->get_image(),
            .subresourceRange =
                {
                    .aspectMask = image->get_aspect_mask(),
                    .baseMipLevel = texture_barriers[i].level,
                    .levelCount = texture_barriers[i].level_count,
                    .baseArrayLayer = texture_barriers[i].layer,
                    .layerCount = texture_barriers[i].layer_count,
                },
        };

        src_stages |= texture_barriers[i].src_stages;
        dst_stages |= texture_barriers[i].dst_stages;
    }

    vkCmdPipelineBarrier(
        m_command_buffer,
        vk_utils::map_pipeline_stage_flags(src_stages),
        vk_utils::map_pipeline_stage_flags(dst_stages),
        0,
        0,
        nullptr,
        buffer_barrier_count,
        vk_buffer_barriers.data(),
        texture_barrier_count,
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

    auto* src_image = static_cast<vk_texture*>(src);
    auto* dst_image = static_cast<vk_texture*>(dst);

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
    const rhi_texture_region& dst_region,
    rhi_filter filter)
{
    auto* src_image = static_cast<vk_texture*>(src);
    auto* dst_image = static_cast<vk_texture*>(dst);

    VkImageBlit image_blit = {};

    image_blit.srcOffsets[0] = {src_region.offset_x, src_region.offset_y, 0};
    image_blit.srcOffsets[1] = {
        static_cast<std::int32_t>(src_region.extent.width),
        static_cast<std::int32_t>(src_region.extent.height),
        1};
    image_blit.srcSubresource.aspectMask = src_region.aspect == 0 ?
                                               src_image->get_aspect_mask() :
                                               vk_utils::map_image_aspect_flags(src_region.aspect);
    image_blit.srcSubresource.mipLevel = src_region.level;
    image_blit.srcSubresource.baseArrayLayer = src_region.layer;
    image_blit.srcSubresource.layerCount = src_region.layer_count;

    image_blit.dstOffsets[0] = {dst_region.offset_x, dst_region.offset_y, 0};
    image_blit.dstOffsets[1] = {
        static_cast<std::int32_t>(dst_region.extent.width),
        static_cast<std::int32_t>(dst_region.extent.height),
        1};
    image_blit.dstSubresource.aspectMask = dst_region.aspect == 0 ?
                                               dst_image->get_aspect_mask() :
                                               vk_utils::map_image_aspect_flags(dst_region.aspect);
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
        vk_utils::map_filter(filter));
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

    VkBufferCopy buffer_copy = {
        .srcOffset = src_region.offset,
        .dstOffset = dst_region.offset,
        .size = src_region.size,
    };

    vkCmdCopyBuffer(m_command_buffer, src_buffer, dst_buffer, 1, &buffer_copy);
}

void vk_command::copy_buffer_to_texture(
    rhi_buffer* buffer,
    const rhi_buffer_region& buffer_region,
    rhi_texture* texture,
    const rhi_texture_region& texture_region)
{
    auto* src_buffer = static_cast<vk_buffer*>(buffer);
    auto* dst_image = static_cast<vk_texture*>(texture);

    VkBufferImageCopy region = {
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource =
            {
                .aspectMask = dst_image->get_aspect_mask(),
                .mipLevel = texture_region.level,
                .baseArrayLayer = texture_region.layer,
                .layerCount = texture_region.layer_count,
            },
        .imageOffset = {0, 0, 0},
        .imageExtent = {texture_region.extent.width, texture_region.extent.height, 1},
    };

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
    m_wait_stages.push_back(vk_utils::map_pipeline_stage_flags(stages));
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
    VkCommandPoolCreateInfo command_pool_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = queue_family_index,
    };

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
    vk_command* command = nullptr;
    if (m_free_commands.empty())
    {
        VkCommandBufferAllocateInfo command_buffer_allocate_info = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = m_command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer command_buffer;
        vk_check(vkAllocateCommandBuffers(
            m_context->get_device(),
            &command_buffer_allocate_info,
            &command_buffer));

        m_commands.push_back(std::make_unique<vk_command>(command_buffer, m_context));
        command = m_commands.back().get();
    }
    else
    {
        command = m_free_commands.back();
        m_free_commands.pop_back();

        command->reset();
    }

    VkCommandBufferBeginInfo command_buffer_begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vk_check(vkBeginCommandBuffer(command->get_command_buffer(), &command_buffer_begin_info));

    return command;
}

void vk_graphics_queue::execute(rhi_command* command, bool sync)
{
    if (sync)
    {
        command->signal(m_fence.get(), ++m_fence_value);
    }

    auto* cast_command = static_cast<vk_command*>(command);

    VkCommandBuffer buffer = cast_command->get_command_buffer();
    vk_check(vkEndCommandBuffer(buffer));

    const auto& signal_semaphores = cast_command->get_signal_fences();
    const auto& signal_values = cast_command->get_signal_values();

    const auto& wait_semaphores = cast_command->get_wait_fences();
    const auto& wait_values = cast_command->get_wait_values();
    const auto& wait_stages = cast_command->get_wait_stages();

    VkTimelineSemaphoreSubmitInfo timeline_info = {
        .sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO,
        .waitSemaphoreValueCount = static_cast<std::uint32_t>(wait_values.size()),
        .pWaitSemaphoreValues = wait_values.data(),
        .signalSemaphoreValueCount = static_cast<std::uint32_t>(signal_values.size()),
        .pSignalSemaphoreValues = signal_values.data(),
    };

    VkSubmitInfo submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = &timeline_info,
        .waitSemaphoreCount = static_cast<std::uint32_t>(wait_semaphores.size()),
        .pWaitSemaphores = wait_semaphores.empty() ? nullptr : wait_semaphores.data(),
        .pWaitDstStageMask = wait_stages.data(),
        .commandBufferCount = 1,
        .pCommandBuffers = &buffer,
        .signalSemaphoreCount = static_cast<std::uint32_t>(signal_semaphores.size()),
        .pSignalSemaphores = signal_semaphores.empty() ? nullptr : signal_semaphores.data(),
    };

    std::scoped_lock lock(m_mutex);

    vk_check(vkQueueSubmit(m_queue, 1, &submit_info, VK_NULL_HANDLE));

    if (sync)
    {
        m_fence->wait(m_fence_value);
        m_free_commands.push_back(static_cast<vk_command*>(command));
    }
    else
    {
        m_active_commands[m_context->get_frame_resource_index()].push_back(cast_command);
    }
}

void vk_graphics_queue::begin_frame()
{
    auto& commands = m_active_commands[m_context->get_frame_resource_index()];
    for (vk_command* command : commands)
    {
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
    VkPresentInfoKHR present_info = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &wait_semaphore,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &image_index,
    };

    VkResult result = vkQueuePresentKHR(m_queue, &present_info);
    if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR)
    {
        vk_check(result);
    }
}
} // namespace violet::vk
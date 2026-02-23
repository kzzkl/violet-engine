#include "vk_command.hpp"
#include "vk_context.hpp"
#include "vk_layout.hpp"
#include "vk_pipeline.hpp"
#include "vk_query.hpp"
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
    std::uint32_t attachment_count,
    const rhi_texture_extent& render_area)
{
    assert(m_current_render_pass == nullptr);

    m_current_render_pass = static_cast<vk_render_pass*>(render_pass);
    m_current_render_pass->begin(m_command_buffer, attachments, attachment_count, render_area);
}

void vk_command::end_render_pass()
{
    m_current_render_pass->end(m_command_buffer);
    m_current_render_pass = nullptr;
}

void vk_command::set_pipeline(rhi_raster_pipeline* raster_pipeline)
{
    auto* pipeline = static_cast<vk_raster_pipeline*>(raster_pipeline);
    if (m_current_pipeline == pipeline->get_pipeline())
    {
        return;
    }

    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
    m_current_pipeline = pipeline->get_pipeline();
    m_current_bind_point = VK_PIPELINE_BIND_POINT_GRAPHICS;
}

void vk_command::set_pipeline(rhi_compute_pipeline* compute_pipeline)
{
    auto* pipeline = static_cast<vk_compute_pipeline*>(compute_pipeline);
    if (m_current_pipeline == pipeline->get_pipeline())
    {
        return;
    }

    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->get_pipeline());

    m_current_pipeline_layout = pipeline->get_pipeline_layout();
    m_current_pipeline = pipeline->get_pipeline();
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

void vk_command::set_constant(const void* data, std::size_t size, std::size_t offset)
{
    assert(m_current_pipeline_layout->get_push_constant_size() >= size);

    vkCmdPushConstants(
        m_command_buffer,
        m_current_pipeline_layout->get_layout(),
        m_current_pipeline_layout->get_push_constant_stages(),
        static_cast<std::uint32_t>(offset),
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

    std::vector<VkBufferMemoryBarrier2> vk_buffer_barriers(buffer_barrier_count);
    for (std::size_t i = 0; i < buffer_barrier_count; ++i)
    {
        vk_buffer_barriers[i] = {
            .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
            .srcStageMask = vk_utils::map_pipeline_stage_flags(buffer_barriers[i].src_stages),
            .srcAccessMask = vk_utils::map_access_flags(buffer_barriers[i].src_access),
            .dstStageMask = vk_utils::map_pipeline_stage_flags(buffer_barriers[i].dst_stages),
            .dstAccessMask = vk_utils::map_access_flags(buffer_barriers[i].dst_access),
            .buffer = static_cast<vk_buffer*>(buffer_barriers[i].buffer)->get_buffer(),
            .offset = buffer_barriers[i].offset,
            .size = buffer_barriers[i].size,
        };
    }

    std::vector<VkImageMemoryBarrier2> vk_image_barriers(texture_barrier_count);
    for (std::size_t i = 0; i < texture_barrier_count; ++i)
    {
        auto* image = static_cast<vk_texture*>(texture_barriers[i].texture);

        vk_image_barriers[i] = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
            .srcStageMask = vk_utils::map_pipeline_stage_flags(texture_barriers[i].src_stages),
            .srcAccessMask = vk_utils::map_access_flags(texture_barriers[i].src_access),
            .dstStageMask = vk_utils::map_pipeline_stage_flags(texture_barriers[i].dst_stages),
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
    }

    VkDependencyInfo dependency_info = {
        .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
        .bufferMemoryBarrierCount = buffer_barrier_count,
        .pBufferMemoryBarriers = vk_buffer_barriers.data(),
        .imageMemoryBarrierCount = texture_barrier_count,
        .pImageMemoryBarriers = vk_image_barriers.data(),
    };

    vkCmdPipelineBarrier2(m_command_buffer, &dependency_info);
}

void vk_command::clear_texture(
    rhi_texture* texture,
    rhi_clear_value clear_value,
    const rhi_texture_region* regions,
    std::uint32_t region_count)
{
    auto* image = static_cast<vk_texture*>(texture);

    std::vector<VkImageSubresourceRange> ranges;
    ranges.reserve(region_count);
    for (std::size_t i = 0; i < region_count; ++i)
    {
        ranges.push_back({
            .aspectMask = regions[i].aspect == 0 ?
                              image->get_aspect_mask() :
                              vk_utils::map_image_aspect_flags(regions[i].aspect),
            .baseMipLevel = regions[i].level,
            .levelCount = 1,
            .baseArrayLayer = regions[i].layer,
            .layerCount = regions[i].layer_count,
        });
    }

    if (image->get_aspect_mask() == VK_IMAGE_ASPECT_COLOR_BIT)
    {
        VkClearColorValue value;
        value.uint32[0] = clear_value.color.uint32[0];
        value.uint32[1] = clear_value.color.uint32[1];
        value.uint32[2] = clear_value.color.uint32[2];
        value.uint32[3] = clear_value.color.uint32[3];

        vkCmdClearColorImage(
            m_command_buffer,
            image->get_image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &value,
            region_count,
            ranges.data());
    }
    else
    {
        VkClearDepthStencilValue value = {
            .depth = clear_value.depth_stencil.depth,
            .stencil = clear_value.depth_stencil.stencil,
        };

        vkCmdClearDepthStencilImage(
            m_command_buffer,
            image->get_image(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            &value,
            region_count,
            ranges.data());
    }
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
    image_copy.extent = {
        .width = src_region.extent.width,
        .height = src_region.extent.height,
        .depth = 1,
    };

    image_copy.srcOffset = {
        .x = src_region.offset_x,
        .y = src_region.offset_y,
        .z = 0,
    };
    image_copy.srcSubresource.aspectMask = src_image->get_aspect_mask();
    image_copy.srcSubresource.mipLevel = src_region.level;
    image_copy.srcSubresource.baseArrayLayer = src_region.layer;
    image_copy.srcSubresource.layerCount = src_region.layer_count;

    image_copy.dstOffset = {
        .x = dst_region.offset_x,
        .y = dst_region.offset_y,
        .z = 0,
    };
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

    image_blit.srcOffsets[0] = {
        .x = src_region.offset_x,
        .y = src_region.offset_y,
        .z = 0,
    };
    image_blit.srcOffsets[1] = {
        .x = static_cast<std::int32_t>(src_region.extent.width),
        .y = static_cast<std::int32_t>(src_region.extent.height),
        .z = 1,
    };
    image_blit.srcSubresource.aspectMask = src_region.aspect == 0 ?
                                               src_image->get_aspect_mask() :
                                               vk_utils::map_image_aspect_flags(src_region.aspect);
    image_blit.srcSubresource.mipLevel = src_region.level;
    image_blit.srcSubresource.baseArrayLayer = src_region.layer;
    image_blit.srcSubresource.layerCount = src_region.layer_count;

    image_blit.dstOffsets[0] = {
        .x = dst_region.offset_x,
        .y = dst_region.offset_y,
        .z = 0,
    };
    image_blit.dstOffsets[1] = {
        .x = static_cast<std::int32_t>(dst_region.extent.width),
        .y = static_cast<std::int32_t>(dst_region.extent.height),
        .z = 1,
    };
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

void vk_command::write_timestamp(
    rhi_query_pool* query_pool,
    std::uint32_t index,
    rhi_pipeline_stage_flag stage)
{
    auto* pool = static_cast<vk_query_pool*>(query_pool);

    vkCmdWriteTimestamp2(
        m_command_buffer,
        stage == RHI_PIPELINE_STAGE_BEGIN ? VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT :
                                            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT,
        pool->get_query_pool(),
        index);
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

void vk_graphics_queue::execute(rhi_command_batch* batchs, std::uint32_t batch_count)
{
    std::vector<VkSubmitInfo2> submit_infos(batch_count);
    std::vector<VkCommandBufferSubmitInfo> commands;
    std::vector<VkSemaphoreSubmitInfo> signal_semaphores;
    std::vector<VkSemaphoreSubmitInfo> wait_semaphores;

    std::size_t command_count = 0;
    std::size_t signal_count = 0;
    std::size_t wait_count = 0;
    for (std::uint32_t i = 0; i < batch_count; ++i)
    {
        const auto& batch = batchs[i];
        command_count += batch.command_count;
        signal_count += batch.signal_fence_count;
        wait_count += batch.wait_fence_count;
    }
    commands.reserve(command_count);
    signal_semaphores.reserve(signal_count);
    wait_semaphores.reserve(wait_count);

    for (std::uint32_t i = 0; i < batch_count; ++i)
    {
        const auto& batch = batchs[i];

        std::size_t command_offset = commands.size();
        std::size_t signal_offset = signal_semaphores.size();
        std::size_t wait_offset = wait_semaphores.size();

        for (std::uint32_t j = 0; j < batch.command_count; ++j)
        {
            auto* command = static_cast<vk_command*>(batch.commands[j]);

            vk_check(vkEndCommandBuffer(command->get_command_buffer()));

            commands.push_back({
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
                .commandBuffer = command->get_command_buffer(),
            });

            m_active_commands[m_context->get_frame_resource_index()].push_back(command);
        }

        for (std::uint32_t j = 0; j < batch.wait_fence_count; ++j)
        {
            const auto& fence = batch.wait_fences[j];
            wait_semaphores.push_back({
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = static_cast<vk_fence*>(fence.fence)->get_semaphore(),
                .value = fence.value,
                .stageMask = vk_utils::map_pipeline_stage_flags(fence.stages),
            });
        }

        for (std::uint32_t j = 0; j < batch.signal_fence_count; ++j)
        {
            const auto& fence = batch.signal_fences[j];
            signal_semaphores.push_back({
                .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
                .semaphore = static_cast<vk_fence*>(fence.fence)->get_semaphore(),
                .value = fence.value,
                .stageMask = vk_utils::map_pipeline_stage_flags(fence.stages),
            });
        }

        submit_infos[i] = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
            .waitSemaphoreInfoCount = batch.wait_fence_count,
            .pWaitSemaphoreInfos = wait_semaphores.data() + wait_offset,
            .commandBufferInfoCount = batch.command_count,
            .pCommandBufferInfos = commands.data() + command_offset,
            .signalSemaphoreInfoCount = batch.signal_fence_count,
            .pSignalSemaphoreInfos = signal_semaphores.data() + signal_offset,
        };
    }

    std::scoped_lock lock(m_mutex);

    vk_check(vkQueueSubmit2(
        m_queue,
        static_cast<std::uint32_t>(submit_infos.size()),
        submit_infos.data(),
        VK_NULL_HANDLE));
}

void vk_graphics_queue::execute_sync(rhi_command* command)
{
    ++m_fence_value;

    VkCommandBuffer command_buffer = static_cast<vk_command*>(command)->get_command_buffer();
    vk_check(vkEndCommandBuffer(command_buffer));

    VkCommandBufferSubmitInfo command_buffer_submit_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
        .commandBuffer = command_buffer,
    };

    VkSemaphoreSubmitInfo signal_semaphore_submit_info = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
        .semaphore = m_fence->get_semaphore(),
        .value = m_fence_value,
        .stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
    };

    VkSubmitInfo2 submit_info = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &command_buffer_submit_info,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signal_semaphore_submit_info,
    };

    std::scoped_lock lock(m_mutex);

    vk_check(vkQueueSubmit2(m_queue, 1, &submit_info, VK_NULL_HANDLE));

    m_fence->wait(m_fence_value);
    m_free_commands.push_back(static_cast<vk_command*>(command));
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
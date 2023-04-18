#include "vk_command.hpp"
#include "vk_pipeline.hpp"
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

void vk_command::begin(
    render_pass_interface* render_pass,
    resource_interface* const* attachments,
    std::size_t attachment_count)
{
    VIOLET_VK_ASSERT(m_current_render_pass == nullptr);

    m_current_render_pass = static_cast<vk_render_pass*>(render_pass);

    std::vector<VkImageView> image_views(attachment_count);
    for (std::size_t i = 0; i < attachment_count; ++i)
        image_views[i] = static_cast<vk_image*>(attachments[i])->get_image_view();
    resource_extent extent = attachments[0]->get_extent();

    VkClearValue clear_value = {};
    VkFramebuffer framebuffer = m_rhi->get_framebuffer_cache()->get_framebuffer(
        m_current_render_pass->get_render_pass(),
        image_views.data(),
        attachment_count,
        extent.width,
        extent.height,
        &clear_value);

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.renderPass = m_current_render_pass->get_render_pass();
    render_pass_begin_info.framebuffer = framebuffer;
    render_pass_begin_info.renderArea.offset = {0, 0};
    render_pass_begin_info.renderArea.extent = {extent.width, extent.height};
    render_pass_begin_info.clearValueCount = 1;
    render_pass_begin_info.pClearValues = &clear_value;

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
}

void vk_command::next()
{
    vkCmdNextSubpass(m_command_buffer, VK_SUBPASS_CONTENTS_INLINE);
}

void vk_command::set_pipeline(render_pipeline_interface* render_pipeline)
{
    vk_render_pipeline* pipeline = static_cast<vk_render_pipeline*>(render_pipeline);
    vkCmdBindPipeline(m_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->get_pipeline());
}

void vk_command::set_parameter(std::size_t index, pipeline_parameter_interface* parameter)
{
}

void vk_command::set_viewport(
    float x,
    float y,
    float width,
    float height,
    float min_depth,
    float max_depth)
{
}

void vk_command::set_scissor(const scissor_extent* extents, std::size_t size)
{
}

void vk_command::set_input_assembly_state(
    resource_interface* const* vertex_buffers,
    std::size_t vertex_buffer_count,
    resource_interface* index_buffer,
    primitive_topology primitive_topology)
{
}

void vk_command::draw(std::size_t vertex_start, std::size_t vertex_end)
{
}

void vk_command::draw_indexed(
    std::size_t index_start,
    std::size_t index_end,
    std::size_t vertex_base)
{
}

void vk_command::clear_render_target(resource_interface* render_target, const float4& color)
{
}

void vk_command::clear_depth_stencil(
    resource_interface* depth_stencil,
    bool clear_depth,
    float depth,
    bool clear_stencil,
    std::uint8_t stencil)
{
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
}

vk_command_queue::~vk_command_queue()
{
    vkDestroyCommandPool(m_rhi->get_device(), m_command_pool, nullptr);
}

vk_command* vk_command_queue::allocate_command()
{
    if (m_free_commands.empty())
        allocate_command_buffer(4);

    vk_command* command = m_free_commands.back();
    m_free_commands.pop_back();
    m_active_commands[m_rhi->get_frame_resource_index()].push_back(command);

    VkCommandBufferBeginInfo command_buffer_begin_info = {};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    throw_if_failed(
        vkBeginCommandBuffer(command->get_command_buffer(), &command_buffer_begin_info));

    return command;
}

void vk_command_queue::allocate_command_buffer(std::uint32_t count)
{
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandPool = m_command_pool;
    command_buffer_allocate_info.commandBufferCount = count;

    std::vector<VkCommandBuffer> command_buffers(command_buffer_allocate_info.commandBufferCount);
    throw_if_failed(vkAllocateCommandBuffers(m_rhi->get_device(), nullptr, command_buffers.data()));

    for (VkCommandBuffer command_buffer : command_buffers)
    {
        std::unique_ptr<vk_command> command = std::make_unique<vk_command>(command_buffer, m_rhi);
        m_free_commands.push_back(command.get());
        m_commands.push_back(std::move(command));
    }
}
} // namespace violet::vk
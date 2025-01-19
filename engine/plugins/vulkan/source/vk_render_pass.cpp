#include "vk_render_pass.hpp"
#include "vk_context.hpp"
#include "vk_framebuffer.hpp"
#include "vk_resource.hpp"
#include "vk_utils.hpp"
#include <cassert>
#include <vector>

namespace violet::vk
{
vk_render_pass::vk_render_pass(const rhi_render_pass_desc& desc, vk_context* context)
    : m_context(context)
{
    auto map_load_op = [](rhi_attachment_load_op op)
    {
        switch (op)
        {
        case RHI_ATTACHMENT_LOAD_OP_LOAD:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case RHI_ATTACHMENT_LOAD_OP_CLEAR:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case RHI_ATTACHMENT_LOAD_OP_DONT_CARE:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        default:
            throw std::runtime_error("Invalid load op.");
        }
    };

    auto map_store_op = [](rhi_attachment_store_op op)
    {
        switch (op)
        {
        case RHI_ATTACHMENT_STORE_OP_STORE:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case RHI_ATTACHMENT_STORE_OP_DONT_CARE:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        default:
            throw std::runtime_error("Invalid store op.");
        }
    };

    std::vector<VkAttachmentDescription> attachments;

    auto add_attachment = [&](const rhi_attachment_desc& desc)
    {
        attachments.emplace_back(VkAttachmentDescription{
            .format = vk_utils::map_format(desc.format),
            .samples = vk_utils::map_sample_count(desc.samples),
            .loadOp = map_load_op(desc.load_op),
            .storeOp = map_store_op(desc.store_op),
            .stencilLoadOp = map_load_op(desc.stencil_load_op),
            .stencilStoreOp = map_store_op(desc.stencil_store_op),
            .initialLayout = vk_utils::map_layout(desc.initial_layout),
            .finalLayout = vk_utils::map_layout(desc.final_layout),
        });
    };

    std::vector<VkAttachmentReference> color;
    VkAttachmentReference depth_stencil;
    bool has_depth_stencil = false;

    for (std::size_t i = 0; i < desc.attachment_count; ++i)
    {
        add_attachment(desc.attachments[i]);

        if (desc.attachments[i].type == RHI_ATTACHMENT_RENDER_TARGET)
        {
            color.emplace_back(VkAttachmentReference{
                .attachment = static_cast<std::uint32_t>(i),
                .layout = vk_utils::map_layout(desc.attachments[i].layout),
            });

            ++m_render_target_count;
        }
        else
        {
            assert(has_depth_stencil == false);

            depth_stencil.attachment = static_cast<std::uint32_t>(i);
            depth_stencil.layout = vk_utils::map_layout(desc.attachments[i].layout);

            has_depth_stencil = true;
        }

        m_attachment_layout.push_back(desc.attachments[i].type);
    }

    VkSubpassDescription subpass = {
        .colorAttachmentCount = static_cast<std::uint32_t>(color.size()),
        .pColorAttachments = color.data(),
        .pDepthStencilAttachment = has_depth_stencil ? &depth_stencil : nullptr,
        .preserveAttachmentCount = 0,
    };

    std::vector<VkSubpassDependency> dependencies = {
        {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = vk_utils::map_pipeline_stage_flags(desc.begin_dependency.src_stages),
            .dstStageMask = vk_utils::map_pipeline_stage_flags(desc.begin_dependency.dst_stages),
            .srcAccessMask = vk_utils::map_access_flags(desc.begin_dependency.src_access),
            .dstAccessMask = vk_utils::map_access_flags(desc.begin_dependency.dst_access),
        },
        {
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
            .srcStageMask = vk_utils::map_pipeline_stage_flags(desc.end_dependency.src_stages),
            .dstStageMask = vk_utils::map_pipeline_stage_flags(desc.end_dependency.dst_stages),
            .srcAccessMask = vk_utils::map_access_flags(desc.end_dependency.src_access),
            .dstAccessMask = vk_utils::map_access_flags(desc.end_dependency.dst_access),
        },
    };

    VkRenderPassCreateInfo render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = static_cast<std::uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data(),
    };

    vk_check(
        vkCreateRenderPass(m_context->get_device(), &render_pass_info, nullptr, &m_render_pass));
}

vk_render_pass::~vk_render_pass()
{
    m_context->get_framebuffer_manager()->notify_render_pass_deleted(m_render_pass);
    vkDestroyRenderPass(m_context->get_device(), m_render_pass, nullptr);
}

void vk_render_pass::begin(
    VkCommandBuffer command_buffer,
    const rhi_attachment* attachments,
    std::size_t attachment_count)
{
    assert(m_attachment_layout.size() == attachment_count);

    std::vector<VkImageView> image_views;
    image_views.reserve(attachment_count);

    std::vector<VkClearValue> clear_values(attachment_count);

    VkExtent2D extent = {};

    for (std::size_t i = 0; i < attachment_count; ++i)
    {
        if (m_attachment_layout[i] == RHI_ATTACHMENT_RENDER_TARGET)
        {
            const auto* rtv = static_cast<const vk_texture_rtv*>(attachments[i].rtv);

            image_views.push_back(rtv->get_image_view());

            auto e = rtv->get_texture()->get_extent();
            extent = {e.width, e.height};

            std::memcpy(
                clear_values[i].color.float32,
                attachments[i].clear_value.color,
                4 * sizeof(float));
        }
        else
        {
            const auto* dsv = static_cast<const vk_texture_dsv*>(attachments[i].dsv);

            image_views.push_back(dsv->get_image_view());

            auto e = dsv->get_texture()->get_extent();
            extent = {e.width, e.height};

            clear_values[i].depthStencil.depth = attachments[i].clear_value.depth;
            clear_values[i].depthStencil.stencil = attachments[i].clear_value.stencil;
        }
    }

    VkFramebuffer framebuffer = m_context->get_framebuffer_manager()->allocate_framebuffer(
        m_render_pass,
        image_views,
        extent);

    VkRenderPassBeginInfo render_pass_begin_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = m_render_pass,
        .framebuffer = framebuffer,
        .renderArea =
            {
                .offset = {0, 0},
                .extent = {extent.width, extent.height},
            },
        .clearValueCount = static_cast<std::uint32_t>(clear_values.size()),
        .pClearValues = clear_values.data(),
    };

    vkCmdBeginRenderPass(command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void vk_render_pass::end(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
}
} // namespace violet::vk
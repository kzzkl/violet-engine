#include "vk_render_pass.hpp"
#include "vk_context.hpp"
#include "vk_framebuffer.hpp"
#include "vk_resource.hpp"
#include "vk_utils.hpp"
#include <array>
#include <cassert>

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

    std::vector<VkAttachmentDescription2> attachments;

    auto add_attachment = [&](const rhi_attachment_desc& desc)
    {
        attachments.emplace_back(
            VkAttachmentDescription2{
                .sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2,
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

    std::vector<VkAttachmentReference2> color;
    VkAttachmentReference2 depth_stencil = {
        .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
    };
    bool has_depth_stencil = false;

    for (std::size_t i = 0; i < desc.attachment_count; ++i)
    {
        add_attachment(desc.attachments[i]);

        if (desc.attachments[i].type == RHI_ATTACHMENT_TYPE_RENDER_TARGET)
        {
            color.emplace_back(
                VkAttachmentReference2{
                    .sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2,
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

    VkSubpassDescription2 subpass = {
        .sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2,
        .colorAttachmentCount = static_cast<std::uint32_t>(color.size()),
        .pColorAttachments = color.data(),
        .pDepthStencilAttachment = has_depth_stencil ? &depth_stencil : nullptr,
        .preserveAttachmentCount = 0,
    };

    VkRenderPassCreateInfo2 render_pass_info = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2,
        .attachmentCount = static_cast<std::uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    std::array<VkSubpassDependency2, 2> dependencies;
    std::array<VkMemoryBarrier2, 2> memory_barriers;
    if (desc.attachment_count != 0)
    {
        memory_barriers[0] = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .srcStageMask = vk_utils::map_pipeline_stage_flags(desc.begin_dependency.src_stages),
            .srcAccessMask = vk_utils::map_access_flags(desc.begin_dependency.src_access),
            .dstStageMask = vk_utils::map_pipeline_stage_flags(desc.begin_dependency.dst_stages),
            .dstAccessMask = vk_utils::map_access_flags(desc.begin_dependency.dst_access),
        };
        dependencies[0] = {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .pNext = memory_barriers.data(),
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
        };

        memory_barriers[1] = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
            .srcStageMask = vk_utils::map_pipeline_stage_flags(desc.end_dependency.src_stages),
            .srcAccessMask = vk_utils::map_access_flags(desc.end_dependency.src_access),
            .dstStageMask = vk_utils::map_pipeline_stage_flags(desc.end_dependency.dst_stages),
            .dstAccessMask = vk_utils::map_access_flags(desc.end_dependency.dst_access),
        };
        dependencies[1] = {
            .sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2,
            .pNext = memory_barriers.data() + 1,
            .srcSubpass = 0,
            .dstSubpass = VK_SUBPASS_EXTERNAL,
        };

        render_pass_info.dependencyCount = static_cast<std::uint32_t>(dependencies.size());
        render_pass_info.pDependencies = dependencies.data();
    }

    vk_check(
        vkCreateRenderPass2(m_context->get_device(), &render_pass_info, nullptr, &m_render_pass));
}

vk_render_pass::~vk_render_pass()
{
    m_context->get_framebuffer_manager()->notify_render_pass_deleted(m_render_pass);
    vkDestroyRenderPass(m_context->get_device(), m_render_pass, nullptr);
}

void vk_render_pass::begin(
    VkCommandBuffer command_buffer,
    const rhi_attachment* attachments,
    std::size_t attachment_count,
    const rhi_texture_extent& render_area)
{
    assert(m_attachment_layout.size() == attachment_count);

    std::vector<VkImageView> image_views;
    image_views.reserve(attachment_count);

    std::vector<VkClearValue> clear_values(attachment_count);

    VkExtent2D extent = {};

    for (std::size_t i = 0; i < attachment_count; ++i)
    {
        if (m_attachment_layout[i] == RHI_ATTACHMENT_TYPE_RENDER_TARGET)
        {
            const auto* rtv = static_cast<const vk_texture_rtv*>(attachments[i].rtv);

            image_views.push_back(rtv->get_image_view());

            auto e = rtv->get_texture()->get_extent();
            extent = {.width = e.width, .height = e.height};

            std::memcpy(
                clear_values[i].color.float32,
                attachments[i].clear_value.color.float32,
                4 * sizeof(float));
        }
        else
        {
            const auto* dsv = static_cast<const vk_texture_dsv*>(attachments[i].dsv);

            image_views.push_back(dsv->get_image_view());

            auto e = dsv->get_texture()->get_extent();
            extent = {.width = e.width, .height = e.height};

            clear_values[i].depthStencil.depth = attachments[i].clear_value.depth_stencil.depth;
            clear_values[i].depthStencil.stencil = attachments[i].clear_value.depth_stencil.stencil;
        }
    }

    if (attachment_count == 0)
    {
        assert(render_area.width != 0 && render_area.height != 0);
        extent = {.width = render_area.width, .height = render_area.height};
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
                .extent = extent,
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
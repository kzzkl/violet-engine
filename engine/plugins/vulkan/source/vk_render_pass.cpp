#include "vk_render_pass.hpp"
#include "vk_rhi.hpp"
#include "vk_util.hpp"
#include <vector>

namespace violet::vk
{
vk_render_pass::vk_render_pass(const rhi_render_pass_desc& desc, vk_rhi* rhi)
    : m_extent{512, 512},
      m_rhi(rhi)
{
    auto map_load_op = [](rhi_attachment_load_op op) {
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

    auto map_store_op = [](rhi_attachment_store_op op) {
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
    for (std::size_t i = 0; i < desc.attachment_count; ++i)
    {
        VkAttachmentDescription attachment = {};
        attachment.format = vk_util::map_format(desc.attachments[i].format);
        attachment.samples = vk_util::map_sample_count(desc.attachments[i].samples);
        attachment.loadOp = map_load_op(desc.attachments[i].load_op);
        attachment.storeOp = map_store_op(desc.attachments[i].store_op);
        attachment.stencilLoadOp = map_load_op(desc.attachments[i].stencil_load_op);
        attachment.stencilStoreOp = map_store_op(desc.attachments[i].stencil_store_op);
        attachment.initialLayout = vk_util::map_state(desc.attachments[i].initial_state);
        attachment.finalLayout = vk_util::map_state(desc.attachments[i].final_state);

        attachments.push_back(attachment);
    }

    std::vector<VkSubpassDescription> subpasses(desc.subpass_count);
    std::vector<std::vector<VkAttachmentReference>> input(desc.subpass_count);
    std::vector<std::vector<VkAttachmentReference>> color(desc.subpass_count);
    std::vector<std::vector<VkAttachmentReference>> resolve(desc.subpass_count);
    std::vector<VkAttachmentReference> depth(desc.subpass_count);
    for (std::size_t i = 0; i < desc.subpass_count; ++i)
    {
        bool has_resolve_attachment = false;
        for (std::size_t j = 0; j < desc.subpasses[i].reference_count; ++j)
        {
            VkAttachmentReference ref = {};
            ref.attachment = desc.subpasses[i].references[j].index;
            ref.layout = vk_util::map_state(desc.subpasses[i].references[j].state);
            switch (desc.subpasses[i].references[j].type)
            {
            case RHI_ATTACHMENT_REFERENCE_TYPE_INPUT: {
                input[i].push_back(ref);
                break;
            }
            case RHI_ATTACHMENT_REFERENCE_TYPE_COLOR: {
                color[i].push_back(ref);
                break;
            }
            // case ATTACHMENT_REFERENCE_TYPE_DEPTH: {
            //     depth[i].attachment = static_cast<std::uint32_t>(j);
            //     depth[i].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            //     subpasses[i].pDepthStencilAttachment = &depth[i];
            //     break;
            // }
            case RHI_ATTACHMENT_REFERENCE_TYPE_RESOLVE: {
                has_resolve_attachment = true;
                break;
            }
            default:
                throw std::runtime_error("Invalid attachment reference type.");
            }
        }

        if (has_resolve_attachment)
        {
            resolve[i].resize(color[i].size());
            for (std::size_t j = 0; j < desc.subpasses[i].reference_count; ++j)
            {
                std::size_t resolve_index = desc.subpasses[i].references[j].resolve_index;
                resolve[i][resolve_index].attachment = static_cast<std::uint32_t>(j);
            }
            subpasses[i].pResolveAttachments = resolve[i].data();
        }

        subpasses[i].pInputAttachments = input[i].data();
        subpasses[i].inputAttachmentCount = static_cast<std::uint32_t>(input[i].size());
        subpasses[i].pColorAttachments = color[i].data();
        subpasses[i].colorAttachmentCount = static_cast<std::uint32_t>(color[i].size());
    }

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    render_pass_info.pSubpasses = subpasses.data();
    render_pass_info.subpassCount = static_cast<std::uint32_t>(subpasses.size());

    throw_if_failed(
        vkCreateRenderPass(m_rhi->get_device(), &render_pass_info, nullptr, &m_render_pass));
}

vk_render_pass::~vk_render_pass()
{
    vkDestroyRenderPass(m_rhi->get_device(), m_render_pass, nullptr);
}
} // namespace violet::vk
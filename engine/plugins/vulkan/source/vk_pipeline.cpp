#include "vk_pipeline.hpp"
#include "vk_rhi.hpp"
#include "vk_util.hpp"
#include <fstream>

namespace violet::vk
{
vk_render_pass::vk_render_pass(const render_pass_desc& desc, vk_rhi* rhi)
    : m_extent{512, 512},
      m_rhi(rhi)
{
    auto convert_load_op = [](render_attachment_load_op op) {
        switch (op)
        {
        case RENDER_ATTACHMENT_LOAD_OP_LOAD:
            return VK_ATTACHMENT_LOAD_OP_LOAD;
        case RENDER_ATTACHMENT_LOAD_OP_CLEAR:
            return VK_ATTACHMENT_LOAD_OP_CLEAR;
        case RENDER_ATTACHMENT_LOAD_OP_DONT_CARE:
            return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        default:
            throw std::runtime_error("Invalid load op.");
        }
    };

    auto convert_store_op = [](render_attachment_store_op op) {
        switch (op)
        {
        case RENDER_ATTACHMENT_STORE_OP_STORE:
            return VK_ATTACHMENT_STORE_OP_STORE;
        case RENDER_ATTACHMENT_STORE_OP_DONT_CARE:
            return VK_ATTACHMENT_STORE_OP_DONT_CARE;
        default:
            throw std::runtime_error("Invalid store op.");
        }
    };

    std::vector<VkAttachmentDescription> attachments;
    for (std::size_t i = 0; i < desc.attachment_count; ++i)
    {
        VkAttachmentDescription attachment = {};
        attachment.format = convert(desc.attachments[i].format);
        switch (desc.attachments[i].samples)
        {
        case 1:
            attachment.samples = VK_SAMPLE_COUNT_1_BIT;
            break;
        case 2:
            attachment.samples = VK_SAMPLE_COUNT_2_BIT;
            break;
        case 4:
            attachment.samples = VK_SAMPLE_COUNT_4_BIT;
            break;
        case 8:
            attachment.samples = VK_SAMPLE_COUNT_8_BIT;
            break;
        default:
            throw std::runtime_error("Invalid sample count.");
        }

        attachment.loadOp = convert_load_op(desc.attachments[i].load_op);
        attachment.storeOp = convert_store_op(desc.attachments[i].store_op);
        attachment.stencilLoadOp = convert_load_op(desc.attachments[i].stencil_load_op);
        attachment.stencilStoreOp = convert_store_op(desc.attachments[i].stencil_store_op);
        attachment.initialLayout = convert(desc.attachments[i].initial_state);
        attachment.finalLayout = convert(desc.attachments[i].final_state);

        attachments.push_back(attachment);
    }

    std::vector<VkSubpassDescription> subpasses;
    std::vector<std::vector<VkAttachmentReference>> input(desc.pass_count);
    std::vector<std::vector<VkAttachmentReference>> color(desc.pass_count);
    std::vector<std::vector<VkAttachmentReference>> resolve(desc.pass_count);
    std::vector<VkAttachmentReference> depth(desc.pass_count);
    for (std::size_t i = 0; i < desc.pass_count; ++i)
    {
        bool has_resolve_attachment = false;
        for (std::size_t j = 0; j < desc.subpasses[i].reference_count; ++j)
        {
            switch (desc.subpasses[i].references[j].type)
            {
            case RENDER_ATTACHMENT_REFERENCE_TYPE_INPUT: {
                VkAttachmentReference ref = {};
                ref.attachment = desc.subpasses[i].references[j].index;
                ref.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                input[i].push_back(ref);
                break;
            }
            case RENDER_ATTACHMENT_REFERENCE_TYPE_COLOR: {
                VkAttachmentReference ref = {};
                ref.attachment = desc.subpasses[i].references[j].index;
                ref.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                color[i].push_back(ref);
                break;
            }
            case RENDER_ATTACHMENT_REFERENCE_TYPE_DEPTH: {
                depth[i].attachment = static_cast<std::uint32_t>(j);
                depth[i].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                subpasses[i].pDepthStencilAttachment = &depth[i];
                break;
            }
            case RENDER_ATTACHMENT_REFERENCE_TYPE_RESOLVE: {
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
        subpasses[i].colorAttachmentCount = static_cast<std::uint32_t>(input[i].size());
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

vk_pipeline_parameter::vk_pipeline_parameter(const pipeline_parameter_desc& desc)
{
}

vk_pipeline_parameter::~vk_pipeline_parameter()
{
}

void vk_pipeline_parameter::set(std::size_t index, const void* data, size_t size)
{
}

void vk_pipeline_parameter::set(std::size_t index, rhi_resource* texture)
{
}

vk_render_pipeline::vk_render_pipeline(
    const render_pipeline_desc& desc,
    VkExtent2D extent,
    vk_rhi* rhi)
    : m_rhi(rhi)
{
    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = load_shader(desc.vertex_shader);
    vert_shader_stage_info.pName = "vs_main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = load_shader(desc.pixel_shader);
    frag_shader_stage_info.pName = "ps_main";

    VkPipelineShaderStageCreateInfo shader_stage_infos[] = {
        vert_shader_stage_info,
        frag_shader_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.pVertexAttributeDescriptions = nullptr;
    vertex_input_state_info.vertexAttributeDescriptionCount = 0;
    vertex_input_state_info.pVertexBindingDescriptions = nullptr;
    vertex_input_state_info.vertexBindingDescriptionCount = 0;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {};
    input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = extent.width;
    viewport.height = extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = extent;

    VkPipelineViewportStateCreateInfo viewport_info = {};
    viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_info.viewportCount = 1;
    viewport_info.pViewports = &viewport;
    viewport_info.scissorCount = 1;
    viewport_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_info = {};
    rasterization_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_info.depthClampEnable = VK_FALSE;
    rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_info.lineWidth = 1.0f;
    rasterization_state_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_state_info.depthBiasEnable = VK_FALSE;
    rasterization_state_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_info.depthBiasClamp = 0.0f;
    rasterization_state_info.depthBiasSlopeFactor = 0.0f;
    switch (desc.rasterizer.cull_mode)
    {
    case CULL_MODE_NONE:
        rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
        break;
    case CULL_MODE_FRONT:
        rasterization_state_info.cullMode = VK_CULL_MODE_FRONT_BIT;
        break;
    case CULL_MODE_BACK:
        rasterization_state_info.cullMode = VK_CULL_MODE_BACK_BIT;
        break;
    default:
        throw std::runtime_error("Invalid cull mode.");
    }

    VkPipelineMultisampleStateCreateInfo multisample_state_info = {};
    multisample_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_info.sampleShadingEnable = VK_FALSE;
    multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_info.minSampleShading = 1.0f;
    multisample_state_info.pSampleMask = nullptr;
    multisample_state_info.alphaToCoverageEnable = VK_FALSE;
    multisample_state_info.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {};
    depth_stencil_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_info.depthTestEnable = VK_TRUE;
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_info.front = {};
    depth_stencil_state_info.back = {};

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo color_blend_info = {};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.attachmentCount = 1;
    color_blend_info.pAttachments = &color_blend_attachment;
    color_blend_info.blendConstants[0] = 0.0f;
    color_blend_info.blendConstants[1] = 0.0f;
    color_blend_info.blendConstants[2] = 0.0f;
    color_blend_info.blendConstants[3] = 0.0f;

    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.setLayoutCount = static_cast<std::uint32_t>(descriptor_set_layouts.size());
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    throw_if_failed(
        vkCreatePipelineLayout(m_rhi->get_device(), &layout_info, nullptr, &m_pipeline_layout));

    std::vector<VkDynamicState> dynamic_state = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS};

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pDynamicStates = dynamic_state.data();
    dynamic_state_info.dynamicStateCount = static_cast<std::uint32_t>(dynamic_state.size());

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = 2;
    pipeline_info.pStages = shader_stage_infos;
    pipeline_info.pVertexInputState = &vertex_input_state_info;
    pipeline_info.pInputAssemblyState = &input_assembly_state_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterization_state_info;
    pipeline_info.pMultisampleState = &multisample_state_info;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = static_cast<vk_render_pass*>(desc.render_pass)->get_render_pass();
    pipeline_info.subpass = static_cast<std::uint32_t>(desc.render_subpass_index);
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;

    throw_if_failed(vkCreateGraphicsPipelines(
        m_rhi->get_device(),
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        nullptr,
        &m_pipeline));

    vkDestroyShaderModule(m_rhi->get_device(), vert_shader_stage_info.module, nullptr);
    vkDestroyShaderModule(m_rhi->get_device(), frag_shader_stage_info.module, nullptr);
}

vk_render_pipeline::~vk_render_pipeline()
{
    vkDestroyPipeline(m_rhi->get_device(), m_pipeline, nullptr);
    vkDestroyPipelineLayout(m_rhi->get_device(), m_pipeline_layout, nullptr);
}

VkShaderModule vk_render_pipeline::load_shader(std::string_view path)
{
    std::ifstream fin(path.data(), std::ios::binary);
    if (!fin.is_open())
        throw std::runtime_error("Failed to open file!");

    fin.seekg(0, std::ios::end);
    std::size_t buffer_size = fin.tellg();

    std::vector<char> buffer(buffer_size);
    fin.seekg(0);
    fin.read(buffer.data(), buffer_size);
    fin.close();

    VkShaderModuleCreateInfo shader_module_info = {};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.pCode = reinterpret_cast<std::uint32_t*>(buffer.data());
    shader_module_info.codeSize = buffer.size();

    VkShaderModule result = VK_NULL_HANDLE;
    throw_if_failed(
        vkCreateShaderModule(m_rhi->get_device(), &shader_module_info, nullptr, &result));
    return result;
}
} // namespace violet::vk
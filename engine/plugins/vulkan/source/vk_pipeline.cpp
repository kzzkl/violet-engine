#include "vk_pipeline.hpp"
#include "vk_layout.hpp"
#include "vk_render_pass.hpp"
#include "vk_util.hpp"
#include <cassert>

namespace violet::vk
{
namespace
{
std::uint32_t get_vertex_attribute_stride(VkFormat format) noexcept
{
    switch (format)
    {
    case VK_FORMAT_R8_UNORM:
    case VK_FORMAT_R8_SNORM:
    case VK_FORMAT_R8_UINT:
    case VK_FORMAT_R8_SINT:
        return 1;
    case VK_FORMAT_R8G8_UNORM:
    case VK_FORMAT_R8G8_SNORM:
    case VK_FORMAT_R8G8_UINT:
    case VK_FORMAT_R8G8_SINT:
        return 2;
    case VK_FORMAT_R8G8B8_UNORM:
    case VK_FORMAT_R8G8B8_SNORM:
    case VK_FORMAT_R8G8B8_UINT:
    case VK_FORMAT_R8G8B8_SINT:
        return 3;
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R8G8B8A8_SNORM:
    case VK_FORMAT_R8G8B8A8_UINT:
    case VK_FORMAT_R8G8B8A8_SINT:
        return 4;
    case VK_FORMAT_R32_UINT:
    case VK_FORMAT_R32_SINT:
    case VK_FORMAT_R32_SFLOAT:
        return 4;
    case VK_FORMAT_R32G32_UINT:
    case VK_FORMAT_R32G32_SINT:
    case VK_FORMAT_R32G32_SFLOAT:
        return 8;
    case VK_FORMAT_R32G32B32_UINT:
    case VK_FORMAT_R32G32B32_SINT:
    case VK_FORMAT_R32G32B32_SFLOAT:
        return 12;
    case VK_FORMAT_R32G32B32A32_UINT:
    case VK_FORMAT_R32G32B32A32_SINT:
    case VK_FORMAT_R32G32B32A32_SFLOAT:
        return 16;
    default:
        return 0;
    }
}
} // namespace

vk_shader::vk_shader(const rhi_shader_desc& desc, vk_context* context)
    : m_module(VK_NULL_HANDLE),
      m_context(context)
{
    VkShaderModuleCreateInfo shader_module_info = {};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.pCode = reinterpret_cast<std::uint32_t*>(desc.code);
    shader_module_info.codeSize = desc.code_size;

    vk_check(
        vkCreateShaderModule(m_context->get_device(), &shader_module_info, nullptr, &m_module));

    vk_layout_manager* layout_manager = m_context->get_layout_manager();
    for (std::size_t i = 0; i < desc.parameter_count; ++i)
    {
        parameter parameter = {
            .space = desc.parameters[i].space,
            .layout = layout_manager->get_parameter_layout(desc.parameters[i].desc),
        };
        m_parameters.push_back(parameter);
    }
}

vk_shader::~vk_shader()
{
    vkDestroyShaderModule(m_context->get_device(), m_module, nullptr);
}

vk_vertex_shader::vk_vertex_shader(const rhi_shader_desc& desc, vk_context* context)
    : vk_shader(desc, context)
{
    m_vertex_attributes.reserve(desc.vertex.attribute_count);
    for (std::size_t i = 0; i < desc.vertex.attribute_count; ++i)
    {
        vertex_attribute attribute;
        attribute.name = desc.vertex.attributes[i].name;
        attribute.format = desc.vertex.attributes[i].format;
        m_vertex_attributes.push_back(attribute);
    }
}

vk_render_pipeline::vk_render_pipeline(const rhi_render_pipeline_desc& desc, vk_context* context)
    : m_pipeline(VK_NULL_HANDLE),
      m_pipeline_layout(VK_NULL_HANDLE),
      m_context(context)
{
    vk_vertex_shader* vertex_shader = static_cast<vk_vertex_shader*>(desc.vertex_shader);
    vk_fragment_shader* fragment_shader = static_cast<vk_fragment_shader*>(desc.fragment_shader);

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages;

    VkPipelineShaderStageCreateInfo vertex_shader_stage_info = {};
    vertex_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertex_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertex_shader_stage_info.module = vertex_shader->get_module();
    vertex_shader_stage_info.pName = "vs_main";
    shader_stages.push_back(vertex_shader_stage_info);

    if (fragment_shader != nullptr)
    {
        VkPipelineShaderStageCreateInfo fragment_shader_stage_info = {};
        fragment_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragment_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragment_shader_stage_info.module = fragment_shader->get_module();
        fragment_shader_stage_info.pName = "fs_main";
        shader_stages.push_back(fragment_shader_stage_info);
    }

    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

    const auto& vertex_attributes = vertex_shader->get_vertex_attributes();
    for (std::uint32_t i = 0; i < vertex_attributes.size(); ++i)
    {
        VkFormat format = vk_util::map_format(vertex_attributes[i].format);

        VkVertexInputBindingDescription binding = {};
        binding.binding = i;
        binding.stride = get_vertex_attribute_stride(format);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        binding_descriptions.push_back(binding);

        VkVertexInputAttributeDescription attribute = {};
        attribute.binding = binding.binding;
        attribute.format = format;
        attribute.location = i;
        attribute_descriptions.push_back(attribute);
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.pVertexAttributeDescriptions = attribute_descriptions.data();
    vertex_input_state_info.vertexAttributeDescriptionCount =
        static_cast<std::uint32_t>(attribute_descriptions.size());
    vertex_input_state_info.pVertexBindingDescriptions = binding_descriptions.data();
    vertex_input_state_info.vertexBindingDescriptionCount =
        static_cast<std::uint32_t>(binding_descriptions.size());

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {};
    input_assembly_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_info.primitiveRestartEnable = VK_FALSE;
    switch (desc.primitive_topology)
    {
    case RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case RHI_PRIMITIVE_TOPOLOGY_LINE_LIST:
        input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    default:
        throw std::runtime_error("Invalid primitive topology.");
    }

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = 0.0f;
    viewport.height = 0.0f;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};

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
    rasterization_state_info.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterization_state_info.depthBiasEnable = VK_FALSE;
    rasterization_state_info.depthBiasConstantFactor = 0.0f;
    rasterization_state_info.depthBiasClamp = 0.0f;
    rasterization_state_info.depthBiasSlopeFactor = 0.0f;
    switch (desc.rasterizer.cull_mode)
    {
    case RHI_CULL_MODE_NONE:
        rasterization_state_info.cullMode = VK_CULL_MODE_NONE;
        break;
    case RHI_CULL_MODE_FRONT:
        rasterization_state_info.cullMode = VK_CULL_MODE_FRONT_BIT;
        break;
    case RHI_CULL_MODE_BACK:
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
    depth_stencil_state_info.depthTestEnable = desc.depth_stencil.depth_enable;
    depth_stencil_state_info.depthWriteEnable = desc.depth_stencil.depth_write_enable;
    depth_stencil_state_info.depthCompareOp =
        vk_util::map_compare_op(desc.depth_stencil.depth_compare_op);
    depth_stencil_state_info.stencilTestEnable = desc.depth_stencil.stencil_enable;
    depth_stencil_state_info.front = {
        .failOp = vk_util::map_stencil_op(desc.depth_stencil.stencil_front.fail_op),
        .passOp = vk_util::map_stencil_op(desc.depth_stencil.stencil_front.pass_op),
        .depthFailOp = vk_util::map_stencil_op(desc.depth_stencil.stencil_front.depth_fail_op),
        .compareOp = vk_util::map_compare_op(desc.depth_stencil.stencil_front.compare_op),
        .compareMask = 0xff,
        .writeMask = 0xff,
        .reference = desc.depth_stencil.stencil_front.reference,
    };
    depth_stencil_state_info.back = {
        .failOp = vk_util::map_stencil_op(desc.depth_stencil.stencil_back.fail_op),
        .passOp = vk_util::map_stencil_op(desc.depth_stencil.stencil_back.pass_op),
        .depthFailOp = vk_util::map_stencil_op(desc.depth_stencil.stencil_back.depth_fail_op),
        .compareOp = vk_util::map_compare_op(desc.depth_stencil.stencil_back.compare_op),
        .compareMask = 0xff,
        .writeMask = 0xff,
        .reference = desc.depth_stencil.stencil_back.reference,
    };

    auto* render_pass = static_cast<vk_render_pass*>(desc.render_pass);

    std::size_t blend_count = render_pass->get_subpass_info(desc.subpass_index).render_target_count;
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments(blend_count);
    for (std::size_t i = 0; i < blend_count; ++i)
    {
        color_blend_attachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachments[i].blendEnable = desc.blend.attachments[i].enable;

        color_blend_attachments[i].srcColorBlendFactor =
            vk_util::map_blend_factor(desc.blend.attachments[i].src_color_factor);
        color_blend_attachments[i].dstColorBlendFactor =
            vk_util::map_blend_factor(desc.blend.attachments[i].dst_color_factor);
        color_blend_attachments[i].colorBlendOp =
            vk_util::map_blend_op(desc.blend.attachments[i].color_op);

        color_blend_attachments[i].srcAlphaBlendFactor =
            vk_util::map_blend_factor(desc.blend.attachments[i].src_alpha_factor);
        color_blend_attachments[i].dstAlphaBlendFactor =
            vk_util::map_blend_factor(desc.blend.attachments[i].dst_alpha_factor);
        color_blend_attachments[i].alphaBlendOp =
            vk_util::map_blend_op(desc.blend.attachments[i].alpha_op);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_info = {};
    color_blend_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_info.logicOpEnable = VK_FALSE;
    color_blend_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_info.pAttachments = color_blend_attachments.data();
    color_blend_info.attachmentCount = static_cast<std::uint32_t>(color_blend_attachments.size());
    color_blend_info.blendConstants[0] = 0.0f;
    color_blend_info.blendConstants[1] = 0.0f;
    color_blend_info.blendConstants[2] = 0.0f;
    color_blend_info.blendConstants[3] = 0.0f;

    std::vector<vk_parameter_layout*> parameter_layouts;
    for (const auto& parameter : vertex_shader->get_parameters())
    {
        if (parameter_layouts.size() <= parameter.space)
        {
            parameter_layouts.resize(parameter.space + 1);
        }
        parameter_layouts[parameter.space] = parameter.layout;
    }

    if (fragment_shader != nullptr)
    {
        for (const auto& parameter : fragment_shader->get_parameters())
        {
            if (parameter_layouts.size() <= parameter.space)
            {
                parameter_layouts.resize(parameter.space + 1);
            }
            parameter_layouts[parameter.space] = parameter.layout;
        }
    }
    m_pipeline_layout =
        m_context->get_layout_manager()->get_pipeline_layout(parameter_layouts)->get_layout();

    std::vector<VkDynamicState> dynamic_state = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS};

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pDynamicStates = dynamic_state.data();
    dynamic_state_info.dynamicStateCount = static_cast<std::uint32_t>(dynamic_state.size());

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pStages = shader_stages.data();
    pipeline_info.stageCount = static_cast<std::uint32_t>(shader_stages.size());
    pipeline_info.pVertexInputState = &vertex_input_state_info;
    pipeline_info.pInputAssemblyState = &input_assembly_state_info;
    pipeline_info.pViewportState = &viewport_info;
    pipeline_info.pRasterizationState = &rasterization_state_info;
    pipeline_info.pMultisampleState = &multisample_state_info;
    pipeline_info.pColorBlendState = &color_blend_info;
    pipeline_info.pDynamicState = &dynamic_state_info;
    pipeline_info.layout = m_pipeline_layout;
    pipeline_info.renderPass = render_pass->get_render_pass();
    pipeline_info.subpass = static_cast<std::uint32_t>(desc.subpass_index);
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.pDepthStencilState = &depth_stencil_state_info;

    vk_check(vkCreateGraphicsPipelines(
        m_context->get_device(),
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        nullptr,
        &m_pipeline));
}

vk_render_pipeline::~vk_render_pipeline()
{
    vkDestroyPipeline(m_context->get_device(), m_pipeline, nullptr);
}

vk_compute_pipeline::vk_compute_pipeline(const rhi_compute_pipeline_desc& desc, vk_context* context)
    : m_pipeline(VK_NULL_HANDLE),
      m_pipeline_layout(VK_NULL_HANDLE),
      m_context(context)
{
    auto* compute_shader = static_cast<vk_compute_shader*>(desc.compute_shader);

    VkPipelineShaderStageCreateInfo compute_shader_stage_info = {};
    compute_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    compute_shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    compute_shader_stage_info.module = static_cast<vk_shader*>(desc.compute_shader)->get_module();
    compute_shader_stage_info.pName = "cs_main";

    std::vector<vk_parameter_layout*> parameter_layouts;
    for (const auto& parameter : compute_shader->get_parameters())
    {
        if (parameter_layouts.size() <= parameter.space)
        {
            parameter_layouts.resize(parameter.space + 1);
        }
        parameter_layouts[parameter.space] = parameter.layout;
    }
    m_pipeline_layout =
        m_context->get_layout_manager()->get_pipeline_layout(parameter_layouts)->get_layout();

    VkComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;
    pipeline_info.stage = compute_shader_stage_info;
    pipeline_info.layout = m_pipeline_layout;

    vk_check(vkCreateComputePipelines(
        m_context->get_device(),
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        nullptr,
        &m_pipeline));
}

vk_compute_pipeline::~vk_compute_pipeline()
{
    vkDestroyPipeline(m_context->get_device(), m_pipeline, nullptr);
}
} // namespace violet::vk
#include "vk_pipeline.hpp"
#include "vk_context.hpp"
#include "vk_layout.hpp"
#include "vk_render_pass.hpp"
#include "vk_utils.hpp"
#include <algorithm>
#include <cassert>

namespace violet::vk
{
vk_shader::vk_shader(const rhi_shader_desc& desc, vk_context* context)
    : m_entry_point(desc.entry_point),
      m_push_constant_size(static_cast<std::uint32_t>(desc.push_constant_size)),
      m_context(context)
{
    VkShaderModuleCreateInfo shader_module_info = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = desc.code_size,
        .pCode = reinterpret_cast<std::uint32_t*>(desc.code),
    };

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

vk_raster_pipeline::vk_raster_pipeline(const rhi_raster_pipeline_desc& desc, vk_context* context)
    : m_pipeline(VK_NULL_HANDLE),
      m_context(context)
{
    assert(desc.vertex_shader != nullptr);

    std::vector<vk_shader*> shaders = {
        static_cast<vk_shader*>(desc.vertex_shader),
    };

    if (desc.geometry_shader != nullptr)
    {
        shaders.push_back(static_cast<vk_shader*>(desc.geometry_shader));
    }

    if (desc.fragment_shader != nullptr)
    {
        shaders.push_back(static_cast<vk_shader*>(desc.fragment_shader));
    }

    std::vector<VkPipelineShaderStageCreateInfo> shader_stages(shaders.size());
    std::ranges::transform(
        shaders,
        shader_stages.begin(),
        [](vk_shader* shader)
        {
            return VkPipelineShaderStageCreateInfo{
                .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
                .stage = shader->get_stage(),
                .module = shader->get_module(),
                .pName = shader->get_entry_point().data(),
            };
        });

    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;

    const auto& vertex_attributes =
        static_cast<vk_vertex_shader*>(shaders[0])->get_vertex_attributes();
    for (std::uint32_t i = 0; i < vertex_attributes.size(); ++i)
    {
        binding_descriptions.push_back({
            .binding = i,
            .stride =
                static_cast<std::uint32_t>(rhi_get_format_stride(vertex_attributes[i].format)),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        });

        attribute_descriptions.push_back({
            .location = i,
            .binding = i,
            .format = vk_utils::map_format(vertex_attributes[i].format),
        });
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_info = {};
    vertex_input_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_info.pVertexAttributeDescriptions = attribute_descriptions.data();
    vertex_input_state_info.vertexAttributeDescriptionCount =
        static_cast<std::uint32_t>(attribute_descriptions.size());
    vertex_input_state_info.pVertexBindingDescriptions = binding_descriptions.data();
    vertex_input_state_info.vertexBindingDescriptionCount =
        static_cast<std::uint32_t>(binding_descriptions.size());

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .primitiveRestartEnable = VK_FALSE,
    };

    switch (desc.primitive_topology)
    {
    case RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST:
        input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        break;
    case RHI_PRIMITIVE_TOPOLOGY_LINE_LIST:
        input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        break;
    case RHI_PRIMITIVE_TOPOLOGY_POINT_LIST:
        input_assembly_state_info.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        break;
    default:
        throw std::runtime_error("Invalid primitive topology.");
    }

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = 0.0f,
        .height = 0.0f,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    VkRect2D scissor = {};

    VkPipelineViewportStateCreateInfo viewport_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports = &viewport,
        .scissorCount = 1,
        .pScissors = &scissor,
    };

    VkPipelineRasterizationStateCreateInfo rasterization_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    switch (desc.rasterizer_state->polygon_mode)
    {
    case RHI_POLYGON_MODE_FILL:
        rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
        break;
    case RHI_POLYGON_MODE_LINE:
        rasterization_state_info.polygonMode = VK_POLYGON_MODE_LINE;
        break;
    case RHI_POLYGON_MODE_POINT:
        rasterization_state_info.polygonMode = VK_POLYGON_MODE_POINT;
        break;
    default:
        throw std::runtime_error("Invalid polygon mode.");
    }

    switch (desc.rasterizer_state->cull_mode)
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

    VkPipelineMultisampleStateCreateInfo multisample_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 1.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    depth_stencil_state_info.depthTestEnable = desc.depth_stencil_state->depth_enable;
    depth_stencil_state_info.depthWriteEnable = desc.depth_stencil_state->depth_write_enable;
    depth_stencil_state_info.depthCompareOp =
        vk_utils::map_compare_op(desc.depth_stencil_state->depth_compare_op);
    depth_stencil_state_info.stencilTestEnable = desc.depth_stencil_state->stencil_enable;
    depth_stencil_state_info.front = {
        .failOp = vk_utils::map_stencil_op(desc.depth_stencil_state->stencil_front.fail_op),
        .passOp = vk_utils::map_stencil_op(desc.depth_stencil_state->stencil_front.pass_op),
        .depthFailOp =
            vk_utils::map_stencil_op(desc.depth_stencil_state->stencil_front.depth_fail_op),
        .compareOp = vk_utils::map_compare_op(desc.depth_stencil_state->stencil_front.compare_op),
        .compareMask = 0xff,
        .writeMask = 0xff,
        .reference = desc.depth_stencil_state->stencil_front.reference,
    };
    depth_stencil_state_info.back = {
        .failOp = vk_utils::map_stencil_op(desc.depth_stencil_state->stencil_back.fail_op),
        .passOp = vk_utils::map_stencil_op(desc.depth_stencil_state->stencil_back.pass_op),
        .depthFailOp =
            vk_utils::map_stencil_op(desc.depth_stencil_state->stencil_back.depth_fail_op),
        .compareOp = vk_utils::map_compare_op(desc.depth_stencil_state->stencil_back.compare_op),
        .compareMask = 0xff,
        .writeMask = 0xff,
        .reference = desc.depth_stencil_state->stencil_back.reference,
    };

    auto* render_pass = static_cast<vk_render_pass*>(desc.render_pass);

    std::size_t blend_count = render_pass->get_render_target_count();
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments(blend_count);
    for (std::size_t i = 0; i < blend_count; ++i)
    {
        color_blend_attachments[i].colorWriteMask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT;
        color_blend_attachments[i].blendEnable = desc.blend_state->attachments[i].enable;

        color_blend_attachments[i].srcColorBlendFactor =
            vk_utils::map_blend_factor(desc.blend_state->attachments[i].src_color_factor);
        color_blend_attachments[i].dstColorBlendFactor =
            vk_utils::map_blend_factor(desc.blend_state->attachments[i].dst_color_factor);
        color_blend_attachments[i].colorBlendOp =
            vk_utils::map_blend_op(desc.blend_state->attachments[i].color_op);

        color_blend_attachments[i].srcAlphaBlendFactor =
            vk_utils::map_blend_factor(desc.blend_state->attachments[i].src_alpha_factor);
        color_blend_attachments[i].dstAlphaBlendFactor =
            vk_utils::map_blend_factor(desc.blend_state->attachments[i].dst_alpha_factor);
        color_blend_attachments[i].alphaBlendOp =
            vk_utils::map_blend_op(desc.blend_state->attachments[i].alpha_op);
    }

    VkPipelineColorBlendStateCreateInfo color_blend_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = static_cast<std::uint32_t>(color_blend_attachments.size()),
        .pAttachments = color_blend_attachments.data(),
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };

    vk_pipeline_layout_desc pipeline_layout_desc = {};

    for (vk_shader* shader : shaders)
    {
        std::uint32_t constant_size = shader->get_push_constant_size();

        if (constant_size != 0)
        {
            pipeline_layout_desc.push_constant_stages |= shader->get_stage();
            pipeline_layout_desc.push_constant_size = constant_size;
        }

        for (const auto& parameter : shader->get_parameters())
        {
            pipeline_layout_desc.parameters[parameter.space] = parameter.layout;
        }
    }

    assert(pipeline_layout_desc.push_constant_size <= 128);

    m_pipeline_layout = m_context->get_layout_manager()->get_pipeline_layout(pipeline_layout_desc);

    std::vector<VkDynamicState> dynamic_state = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS,
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<std::uint32_t>(dynamic_state.size()),
        .pDynamicStates = dynamic_state.data(),
    };

    VkGraphicsPipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<std::uint32_t>(shader_stages.size()),
        .pStages = shader_stages.data(),
        .pVertexInputState = &vertex_input_state_info,
        .pInputAssemblyState = &input_assembly_state_info,
        .pViewportState = &viewport_info,
        .pRasterizationState = &rasterization_state_info,
        .pMultisampleState = &multisample_state_info,
        .pDepthStencilState = &depth_stencil_state_info,
        .pColorBlendState = &color_blend_info,
        .pDynamicState = &dynamic_state_info,
        .layout = m_pipeline_layout->get_layout(),
        .renderPass = render_pass->get_render_pass(),
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
    };

    vk_check(vkCreateGraphicsPipelines(
        m_context->get_device(),
        VK_NULL_HANDLE,
        1,
        &pipeline_info,
        nullptr,
        &m_pipeline));
}

vk_raster_pipeline::~vk_raster_pipeline()
{
    vkDestroyPipeline(m_context->get_device(), m_pipeline, nullptr);
}

vk_compute_pipeline::vk_compute_pipeline(const rhi_compute_pipeline_desc& desc, vk_context* context)
    : m_context(context)
{
    auto* compute_shader = static_cast<vk_compute_shader*>(desc.compute_shader);

    VkPipelineShaderStageCreateInfo compute_shader_stage_info = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = compute_shader->get_module(),
        .pName = compute_shader->get_entry_point().data(),
    };

    vk_pipeline_layout_desc pipeline_layout_desc = {
        .push_constant_stages = VK_SHADER_STAGE_COMPUTE_BIT,
        .push_constant_size = compute_shader->get_push_constant_size(),
    };
    for (const auto& parameter : compute_shader->get_parameters())
    {
        pipeline_layout_desc.parameters[parameter.space] = parameter.layout;
    }
    m_pipeline_layout = m_context->get_layout_manager()->get_pipeline_layout(pipeline_layout_desc);

    VkComputePipelineCreateInfo pipeline_info = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = compute_shader_stage_info,
        .layout = m_pipeline_layout->get_layout(),
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1,
    };

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
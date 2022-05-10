#include "vk_pipeline.hpp"
#include "vk_context.hpp"
#include "vk_descriptor_pool.hpp"
#include "vk_frame_buffer.hpp"
#include "vk_renderer.hpp"
#include "vk_sampler.hpp"
#include <fstream>

namespace ash::graphics::vk
{
vk_pipeline_parameter_layout::vk_pipeline_parameter_layout(
    const pipeline_parameter_layout_desc& desc)
    : m_ubo_count(0),
      m_cis_count(0)
{
    for (std::size_t i = 0; i < desc.size; ++i)
        m_parameters.push_back(desc.parameters[i]);

    auto device = vk_context::device();

    for (std::size_t i = 0; i < desc.size; ++i)
    {
        switch (desc.parameters[i].type)
        {
        case pipeline_parameter_type::BOOL:
        case pipeline_parameter_type::UINT:
        case pipeline_parameter_type::FLOAT:
        case pipeline_parameter_type::FLOAT2:
        case pipeline_parameter_type::FLOAT3:
        case pipeline_parameter_type::FLOAT4:
        case pipeline_parameter_type::FLOAT4x4:
        case pipeline_parameter_type::FLOAT4x4_ARRAY:
            m_ubo_count = 1;
            break;
        case pipeline_parameter_type::TEXTURE:
            ++m_cis_count;
            break;
        default:
            throw vk_exception("Invalid pipeline parameter type.");
        }
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    if (m_ubo_count != 0)
    {
        VkDescriptorSetLayoutBinding ubo_binding = {};
        ubo_binding.binding = 0;
        ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ubo_binding.descriptorCount = 1;
        ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        ubo_binding.pImmutableSamplers = nullptr;
        bindings.push_back(ubo_binding);
    }

    for (std::size_t i = 0; i < m_cis_count; ++i)
    {
        VkDescriptorSetLayoutBinding cis_binding = {};
        cis_binding.binding = static_cast<std::uint32_t>(m_ubo_count + i);
        cis_binding.descriptorCount = 1;
        cis_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        cis_binding.pImmutableSamplers = nullptr;
        cis_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings.push_back(cis_binding);
    }

    std::vector<VkDescriptorBindingFlags> binding_flags(
        bindings.size(),
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
    VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info = {};
    binding_flags_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
    binding_flags_info.pBindingFlags = binding_flags.data();
    binding_flags_info.bindingCount = static_cast<std::uint32_t>(binding_flags.size());

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());
    descriptor_set_layout_info.pBindings = bindings.data();
    descriptor_set_layout_info.pNext = &binding_flags_info;

    throw_if_failed(vkCreateDescriptorSetLayout(
        device,
        &descriptor_set_layout_info,
        nullptr,
        &m_descriptor_set_layout));
}

vk_pipeline_parameter::vk_pipeline_parameter(pipeline_parameter_layout_interface* layout)
    : m_dirty(0),
      m_last_sync_frame(-1)
{
    auto vk_layout = static_cast<vk_pipeline_parameter_layout*>(layout);
    auto [ubo_count, cis_count] = vk_layout->descriptor_count();
    m_textures.resize(cis_count);

    auto cal_align = [](std::size_t begin, std::size_t align) {
        return (begin + align - 1) & ~(align - 1);
    };

    std::size_t ubo_offset = 0;
    std::size_t texture_offset = 0;
    for (auto& parameter : vk_layout->parameters())
    {
        std::size_t align_address = 0;
        std::size_t size = 0;
        switch (parameter.type)
        {
        case pipeline_parameter_type::BOOL:
            align_address = cal_align(ubo_offset, 4);
            size = sizeof(bool);
            break;
        case pipeline_parameter_type::UINT:
            align_address = cal_align(ubo_offset, 4);
            size = sizeof(std::uint32_t);
            break;
        case pipeline_parameter_type::FLOAT:
            align_address = cal_align(ubo_offset, 4);
            size = sizeof(float);
            break;
        case pipeline_parameter_type::FLOAT2:
            align_address = cal_align(ubo_offset, 8);
            size = sizeof(math::float2);
            break;
        case pipeline_parameter_type::FLOAT3:
            align_address = cal_align(ubo_offset, 16);
            size = sizeof(math::float3);
            break;
        case pipeline_parameter_type::FLOAT4:
            align_address = cal_align(ubo_offset, 16);
            size = sizeof(math::float4);
            break;
        case pipeline_parameter_type::FLOAT4x4:
            align_address = cal_align(ubo_offset, 16);
            size = sizeof(math::float4x4);
            break;
        case pipeline_parameter_type::FLOAT4x4_ARRAY:
            align_address = cal_align(ubo_offset, 16);
            size = sizeof(math::float4x4) * parameter.size;
            break;
        case pipeline_parameter_type::TEXTURE:
            break;
        default:
            throw vk_exception("Invalid pipeline parameter type.");
        }

        if (parameter.type == pipeline_parameter_type::TEXTURE)
        {
            align_address = texture_offset;
            ++texture_offset;
            m_parameter_info.push_back(parameter_info{
                align_address,
                size,
                parameter.type,
                0,
                static_cast<std::uint32_t>(align_address + ubo_count)});
        }
        else
        {
            ubo_offset = align_address + size;
            m_parameter_info.push_back(parameter_info{align_address, size, parameter.type, 0, 0});
        }
    }

    std::size_t frame_resource_count = vk_frame_counter::frame_resource_count();

    std::size_t buffer_size =
        cal_align(ubo_offset, 0x100); // Device limit minUniformBufferOffsetAlignment 0x40
    m_cpu_buffer.resize(buffer_size);
    m_gpu_buffer = std::make_unique<vk_uniform_buffer>(buffer_size * frame_resource_count);

    for (std::size_t i = 0; i < frame_resource_count; ++i)
    {
        auto& descriptor_pool = vk_context::descriptor_pool();
        m_descriptor_set.push_back(descriptor_pool.allocate_descriptor_set(vk_layout->layout()));
    }

    if (ubo_count != 0)
    {
        auto device = vk_context::device();

        VkDescriptorBufferInfo buffer_info = {};
        buffer_info.buffer = m_gpu_buffer->buffer();
        buffer_info.range = buffer_size;

        VkWriteDescriptorSet descriptor_write = {};
        descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_write.dstBinding = 0;
        descriptor_write.dstArrayElement = 0;
        descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptor_write.descriptorCount = 1;
        descriptor_write.pBufferInfo = &buffer_info;
        descriptor_write.pImageInfo = nullptr;
        descriptor_write.pTexelBufferView = nullptr;

        for (std::size_t i = 0; i < frame_resource_count; ++i)
        {
            descriptor_write.dstSet = m_descriptor_set[i];
            buffer_info.offset = i * buffer_size;
            vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, nullptr);
        }
    }
}

void vk_pipeline_parameter::set(std::size_t index, const math::float4x4& value)
{
    math::float4x4 m;
    math::simd::store(math::matrix_simd::transpose(math::simd::load(value)), m);

    std::memcpy(m_cpu_buffer.data() + m_parameter_info[index].offset, &m, sizeof(math::float4x4));
    mark_dirty(index);
}

void vk_pipeline_parameter::set(std::size_t index, const math::float4x4* data, std::size_t size)
{
    math::float4x4 m;
    math::float4x4_simd t;
    for (std::size_t i = 0; i < size; ++i)
    {
        t = math::matrix_simd::transpose(math::simd::load(data[i]));
        math::simd::store(t, m);
        std::memcpy(
            m_cpu_buffer.data() + m_parameter_info[index].offset + sizeof(math::float4x4) * i,
            &m,
            sizeof(math::float4x4));
    }

    mark_dirty(index);
}

void vk_pipeline_parameter::set(std::size_t index, resource_interface* texture)
{
    m_textures[m_parameter_info[index].offset] = static_cast<vk_texture*>(texture);
    mark_dirty(index);
}

void vk_pipeline_parameter::sync()
{
    if (m_last_sync_frame == vk_frame_counter::frame_counter())
        return;

    m_last_sync_frame = vk_frame_counter::frame_counter();

    if (m_dirty == 0)
        return;

    std::size_t resource_index = vk_frame_counter::frame_resource_index();
    for (parameter_info& info : m_parameter_info)
    {
        if (info.dirty == 0)
            continue;

        if (info.type == pipeline_parameter_type::TEXTURE)
        {
            auto texture = m_textures[info.offset];

            VkDescriptorImageInfo image_info = {};
            image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_info.imageView = texture->view();
            image_info.sampler = vk_context::sampler().sampler();

            VkWriteDescriptorSet descriptor_write = {};
            descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptor_write.dstSet = m_descriptor_set[resource_index];
            descriptor_write.dstBinding = info.binding;
            descriptor_write.dstArrayElement = 0;
            descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor_write.descriptorCount = 1;
            descriptor_write.pImageInfo = &image_info;
            vkUpdateDescriptorSets(vk_context::device(), 1, &descriptor_write, 0, nullptr);
        }
        else
        {
            m_gpu_buffer->upload(
                m_cpu_buffer.data() + info.offset,
                info.size,
                m_cpu_buffer.size() * resource_index + info.offset);
        }

        --info.dirty;
        if (info.dirty == 0)
            --m_dirty;
    }
}

VkDescriptorSet vk_pipeline_parameter::descriptor_set() const
{
    return m_descriptor_set[vk_frame_counter::frame_resource_index()];
}

void vk_pipeline_parameter::mark_dirty(std::size_t index)
{
    if (m_parameter_info[index].dirty == 0)
        ++m_dirty;

    m_parameter_info[index].dirty = vk_frame_counter::frame_resource_count();
}

vk_pipeline::vk_pipeline(const pipeline_desc& desc, VkRenderPass render_pass, std::size_t index)
{
    auto device = vk_context::device();

    // Shader.
    VkPipelineShaderStageCreateInfo vert_stage_info = {};
    vert_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_stage_info.module = load_shader(desc.vertex_shader);
    vert_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo frag_stage_info = {};
    frag_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_stage_info.module = load_shader(desc.pixel_shader);
    frag_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_stage_info[] = {vert_stage_info, frag_stage_info};

    // Vertex input.
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription vertex_binding = {};
    vertex_binding.binding = 0;
    vertex_binding.stride = 0;
    vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    std::vector<VkVertexInputAttributeDescription> vertex_attributes;
    std::uint32_t location = 0;
    std::uint32_t offset = 0;
    for (std::size_t i = 0; i < desc.vertex_attribute_count; ++i)
    {
        VkVertexInputAttributeDescription attribute;
        attribute.binding = 0;
        attribute.location = location;
        attribute.offset = offset;

        ++location;
        switch (desc.vertex_attributes[i].type)
        {
        case vertex_attribute_type::INT:
            attribute.format = VK_FORMAT_R32_SINT;
            offset += sizeof(std::int32_t);
            break;
        case vertex_attribute_type::INT2:
            attribute.format = VK_FORMAT_R32G32_SINT;
            offset += sizeof(std::int32_t) * 2;
            break;
        case vertex_attribute_type::INT3:
            attribute.format = VK_FORMAT_R32G32B32_SINT;
            offset += sizeof(std::int32_t) * 3;
            break;
        case vertex_attribute_type::INT4:
            attribute.format = VK_FORMAT_R32G32B32A32_SINT;
            offset += sizeof(std::int32_t) * 4;
            break;
        case vertex_attribute_type::UINT:
            attribute.format = VK_FORMAT_R32_UINT;
            offset += sizeof(std::uint32_t);
            break;
        case vertex_attribute_type::UINT2:
            attribute.format = VK_FORMAT_R32G32_UINT;
            offset += sizeof(std::uint32_t) * 2;
            break;
        case vertex_attribute_type::UINT3:
            attribute.format = VK_FORMAT_R32G32B32_UINT;
            offset += sizeof(std::uint32_t) * 3;
            break;
        case vertex_attribute_type::UINT4:
            attribute.format = VK_FORMAT_R32G32B32A32_UINT;
            offset += sizeof(std::uint32_t) * 4;
            break;
        case vertex_attribute_type::FLOAT:
            attribute.format = VK_FORMAT_R32_SFLOAT;
            offset += sizeof(float);
            break;
        case vertex_attribute_type::FLOAT2:
            attribute.format = VK_FORMAT_R32G32_SFLOAT;
            offset += sizeof(math::float2);
            break;
        case vertex_attribute_type::FLOAT3:
            attribute.format = VK_FORMAT_R32G32B32_SFLOAT;
            offset += sizeof(math::float3);
            break;
        case vertex_attribute_type::FLOAT4:
            attribute.format = VK_FORMAT_R32G32B32A32_SFLOAT;
            offset += sizeof(math::float4);
            break;
        case vertex_attribute_type::COLOR:
            attribute.format = VK_FORMAT_R8G8B8A8_UINT;
            offset += sizeof(std::uint8_t) * 4;
            break;
        default:
            throw vk_exception("Invalid vertex attribute type.");
        }
        vertex_attributes.push_back(attribute);
    }
    vertex_binding.stride = offset;

    vertex_input_info.vertexBindingDescriptionCount = 1;
    vertex_input_info.pVertexBindingDescriptions = &vertex_binding;
    vertex_input_info.vertexAttributeDescriptionCount =
        static_cast<std::uint32_t>(vertex_attributes.size());
    vertex_input_info.pVertexAttributeDescriptions = vertex_attributes.data();

    // Input assembly.
    VkPipelineInputAssemblyStateCreateInfo input_assembly_info = {};
    input_assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_info.primitiveRestartEnable = VK_FALSE;

    // View port.
    VkExtent2D extent = vk_context::swap_chain().extent();

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(extent.width);
    viewport.height = static_cast<float>(extent.height);
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

    // Rasterization.
    VkPipelineRasterizationStateCreateInfo rasterization_info = {};
    rasterization_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_info.depthClampEnable = VK_FALSE;
    rasterization_info.rasterizerDiscardEnable = VK_FALSE;
    rasterization_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_info.lineWidth = 1.0f;
    rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterization_info.depthBiasEnable = VK_FALSE;
    rasterization_info.depthBiasConstantFactor = 0.0f;
    rasterization_info.depthBiasClamp = 0.0f;
    rasterization_info.depthBiasSlopeFactor = 0.0f;

    // Multisample.
    VkPipelineMultisampleStateCreateInfo multisample_info = {};
    multisample_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_info.sampleShadingEnable = VK_FALSE;
    multisample_info.rasterizationSamples = to_vk_samples(desc.samples);
    multisample_info.minSampleShading = 1.0f;
    multisample_info.pSampleMask = nullptr;
    multisample_info.alphaToCoverageEnable = VK_FALSE;
    multisample_info.alphaToOneEnable = VK_FALSE;

    // Depth stencil.
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil.depthTestEnable = VK_TRUE;
    depth_stencil.depthWriteEnable = VK_TRUE;
    depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depth_stencil.stencilTestEnable = VK_FALSE;
    depth_stencil.front = {}; // Optional
    depth_stencil.back = {};  // Optional

    // Blend.
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

    // Pipeline layout
    std::vector<VkDescriptorSetLayout> layouts;
    for (std::size_t i = 0; i < desc.parameter_count; ++i)
        layouts.push_back(static_cast<vk_pipeline_parameter_layout*>(desc.parameters[i])->layout());

    VkPipelineLayoutCreateInfo pass_layout_info = {};
    pass_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pass_layout_info.setLayoutCount = static_cast<std::uint32_t>(layouts.size());
    pass_layout_info.pSetLayouts = layouts.data();
    throw_if_failed(vkCreatePipelineLayout(device, &pass_layout_info, nullptr, &m_pass_layout));

    // Dynamic state.
    std::vector<VkDynamicState> dynamic_state = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_DEPTH_BOUNDS};

    VkPipelineDynamicStateCreateInfo dynamic_state_info = {};
    dynamic_state_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_info.pDynamicStates = dynamic_state.data();
    dynamic_state_info.dynamicStateCount = static_cast<std::uint32_t>(dynamic_state.size());

    // Create pipeline.
    VkGraphicsPipelineCreateInfo pass_info = {};
    pass_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pass_info.stageCount = 2;
    pass_info.pStages = shader_stage_info;
    pass_info.pVertexInputState = &vertex_input_info;
    pass_info.pInputAssemblyState = &input_assembly_info;
    pass_info.pViewportState = &viewport_info;
    pass_info.pRasterizationState = &rasterization_info;
    pass_info.pMultisampleState = &multisample_info;
    pass_info.pColorBlendState = &color_blend_info;
    pass_info.pDynamicState = &dynamic_state_info;
    pass_info.layout = m_pass_layout;
    pass_info.renderPass = render_pass;
    pass_info.subpass = static_cast<std::uint32_t>(index);
    pass_info.basePipelineHandle = VK_NULL_HANDLE;
    pass_info.pDepthStencilState = &depth_stencil;

    throw_if_failed(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pass_info, nullptr, &m_pipeline));

    vkDestroyShaderModule(device, vert_stage_info.module, nullptr);
    vkDestroyShaderModule(device, frag_stage_info.module, nullptr);
}

VkShaderModule vk_pipeline::load_shader(std::string_view file)
{
    std::ifstream fin(file.data(), std::ios::binary);
    ASH_VK_ASSERT(fin.is_open());

    std::vector<char> shader_data(fin.seekg(0, std::ios::end).tellg());
    fin.seekg(0, std::ios::beg).read(shader_data.data(), shader_data.size());
    fin.close();

    VkShaderModuleCreateInfo shader_info = {};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = shader_data.size();
    shader_info.pCode = reinterpret_cast<const std::uint32_t*>(shader_data.data());

    VkShaderModule result;
    throw_if_failed(vkCreateShaderModule(vk_context::device(), &shader_info, nullptr, &result));

    return result;
}

vk_render_pass::vk_render_pass(const render_pass_desc& desc)
{
    create_pass(desc);

    for (std::size_t i = 0; i < desc.subpass_count; ++i)
        m_pipelines.emplace_back(desc.subpasses[i], m_render_pass, i);
}

vk_render_pass::~vk_render_pass()
{
    auto device = vk_context::device();

    for (auto& pipeline : m_pipelines)
    {
        vkDestroyPipeline(device, pipeline.pipeline(), nullptr);
        vkDestroyPipelineLayout(device, pipeline.layout(), nullptr);
    }

    vkDestroyRenderPass(device, m_render_pass, nullptr);
}

void vk_render_pass::begin(VkCommandBuffer command_buffer, vk_image* render_target)
{
    auto frame_buffer = vk_context::frame_buffer().get_or_create_frame_buffer(this, render_target);

    VkRenderPassBeginInfo pass_info = {};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    pass_info.renderPass = m_render_pass;
    pass_info.framebuffer = frame_buffer->frame_buffer();
    pass_info.renderArea.offset = {0, 0};
    // TODO
    pass_info.renderArea.extent = vk_context::swap_chain().extent();
    pass_info.clearValueCount = static_cast<std::uint32_t>(frame_buffer->clear_values().size());
    pass_info.pClearValues = frame_buffer->clear_values().data();

    vkCmdBeginRenderPass(command_buffer, &pass_info, VK_SUBPASS_CONTENTS_INLINE);

    m_subpass_index = 0;
    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[0].pipeline());
}

void vk_render_pass::end(VkCommandBuffer command_buffer)
{
    vkCmdEndRenderPass(command_buffer);
}

void vk_render_pass::next(VkCommandBuffer command_buffer)
{
    ++m_subpass_index;
    vkCmdNextSubpass(command_buffer, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(
        command_buffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_pipelines[m_subpass_index].pipeline());
}

void vk_render_pass::create_pass(const render_pass_desc& desc)
{
    struct subpass_reference
    {
        std::vector<VkAttachmentReference> input;
        std::vector<VkAttachmentReference> color;
        std::vector<VkAttachmentReference> resolve;
        VkAttachmentReference depth;
    };

    std::vector<VkSubpassDescription> subpasses(desc.subpass_count);
    std::vector<subpass_reference> reference(desc.subpass_count);
    for (std::size_t i = 0; i < desc.subpass_count; ++i)
    {
        bool has_resolve_attachment = false;
        for (std::size_t j = 0; j < desc.subpasses[i].reference_count; ++j)
        {
            switch (desc.subpasses[i].references[j].type)
            {
            case attachment_reference_type::UNUSE:
                break;
            case attachment_reference_type::INPUT: {
                VkAttachmentReference attachment_ref = {};
                attachment_ref.attachment = static_cast<std::uint32_t>(j);
                attachment_ref.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                reference[i].input.push_back(attachment_ref);
                break;
            }
            case attachment_reference_type::COLOR: {
                VkAttachmentReference attachment_ref = {};
                attachment_ref.attachment = static_cast<std::uint32_t>(j);
                attachment_ref.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
                reference[i].color.push_back(attachment_ref);
                break;
            }
            case attachment_reference_type::DEPTH: {
                reference[i].depth.attachment = static_cast<std::uint32_t>(j);
                reference[i].depth.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                subpasses[i].pDepthStencilAttachment = &reference[i].depth;
                break;
            }
            case attachment_reference_type::RESOLVE: {
                has_resolve_attachment = true;
                break;
            }
            default:
                throw vk_exception("Invalid attachment reference.");
            }
        }

        if (has_resolve_attachment)
        {
            VkAttachmentReference attachment_ref = {};
            attachment_ref.attachment = VK_ATTACHMENT_UNUSED;
            attachment_ref.layout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
            reference[i].resolve.resize(reference[i].color.size(), attachment_ref);
            for (std::size_t j = 0; j < desc.subpasses[i].reference_count; ++j)
            {
                if (desc.subpasses[i].references[j].type == attachment_reference_type::RESOLVE)
                {
                    std::size_t resolve_relation = desc.subpasses[i].references[j].resolve_relation;
                    reference[i].resolve[resolve_relation].attachment =
                        static_cast<std::uint32_t>(j);
                }
            }
        }

        subpasses[i].inputAttachmentCount = static_cast<std::uint32_t>(reference[i].input.size());
        subpasses[i].pInputAttachments = reference[i].input.data();
        subpasses[i].colorAttachmentCount = static_cast<std::uint32_t>(reference[i].color.size());
        subpasses[i].pColorAttachments = reference[i].color.data();
        subpasses[i].pResolveAttachments =
            has_resolve_attachment ? reference[i].resolve.data() : nullptr;
        subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    }

    m_frame_buffer_layout =
        std::make_unique<vk_frame_buffer_layout>(desc.attachments, desc.attachment_count);

    std::vector<VkAttachmentDescription> attachments;
    for (auto& attachment : *m_frame_buffer_layout)
        attachments.push_back(attachment.description);

    VkRenderPassCreateInfo pass_info = {};
    pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    pass_info.attachmentCount = static_cast<std::uint32_t>(attachments.size());
    pass_info.pAttachments = attachments.data();
    pass_info.subpassCount = static_cast<std::uint32_t>(subpasses.size());
    pass_info.pSubpasses = subpasses.data();

    throw_if_failed(vkCreateRenderPass(vk_context::device(), &pass_info, nullptr, &m_render_pass));
}
} // namespace ash::graphics::vk
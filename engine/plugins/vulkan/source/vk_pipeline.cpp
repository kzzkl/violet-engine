#include "vk_pipeline.hpp"
#include "vk_render_pass.hpp"
#include "vk_resource.hpp"
#include "vk_util.hpp"
#include <algorithm>
#include <cassert>
#include <fstream>

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

vk_parameter_layout::vk_parameter_layout(const rhi_parameter_desc& desc, vk_context* context)
    : m_layout(VK_NULL_HANDLE),
      m_context(context)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings;

    m_parameter_bindings.resize(desc.binding_count);
    std::size_t uniform_buffer_count = 0;
    std::size_t storage_buffer_count = 0;
    std::size_t image_count = 0;

    for (std::size_t i = 0; i < desc.binding_count; ++i)
    {
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = static_cast<std::uint32_t>(i);
        if (desc.bindings[i].stage & RHI_SHADER_STAGE_FLAG_VERTEX)
            binding.stageFlags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (desc.bindings[i].stage & RHI_SHADER_STAGE_FLAG_FRAGMENT)
            binding.stageFlags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (desc.bindings[i].stage & RHI_SHADER_STAGE_FLAG_COMPUTE)
            binding.stageFlags |= VK_SHADER_STAGE_COMPUTE_BIT;

        switch (desc.bindings[i].type)
        {
        case RHI_PARAMETER_TYPE_UNIFORM: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            binding.pImmutableSamplers = nullptr;

            m_parameter_bindings[i].index = uniform_buffer_count;
            m_parameter_bindings[i].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

            VkDeviceSize uniform_alignment =
                m_context->get_physical_device_properties().limits.minUniformBufferOffsetAlignment;
            m_parameter_bindings[i].uniform_buffer.size =
                (desc.bindings[i].size + uniform_alignment - 1) / uniform_alignment *
                uniform_alignment;

            ++uniform_buffer_count;
            break;
        }
        case RHI_PARAMETER_TYPE_STORAGE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.descriptorCount = 1;
            binding.pImmutableSamplers = nullptr;

            m_parameter_bindings[i].index = storage_buffer_count;
            m_parameter_bindings[i].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

            ++storage_buffer_count;
            break;
        }
        case RHI_PARAMETER_TYPE_TEXTURE: {
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.pImmutableSamplers = nullptr;

            m_parameter_bindings[i].index = image_count;
            m_parameter_bindings[i].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

            ++image_count;
            break;
        }
        default:
            break;
        }

        bindings.push_back(binding);
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_info = {};
    descriptor_set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_info.pBindings = bindings.data();
    descriptor_set_layout_info.bindingCount = static_cast<std::uint32_t>(bindings.size());

    vk_check(vkCreateDescriptorSetLayout(
        m_context->get_device(),
        &descriptor_set_layout_info,
        nullptr,
        &m_layout));
}

vk_parameter_layout::~vk_parameter_layout()
{
    vkDestroyDescriptorSetLayout(m_context->get_device(), m_layout, nullptr);
}

vk_pipeline_layout::vk_pipeline_layout(
    const rhi_parameter_desc* parameters,
    std::size_t parameter_count,
    vk_context* context)
    : m_layout(VK_NULL_HANDLE),
      m_context(context)
{
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    for (std::size_t i = 0; i < parameter_count; ++i)
    {
        vk_parameter_layout* layout =
            context->get_layout_manager()->get_parameter_layout(parameters[i]);
        descriptor_set_layouts.push_back(layout->get_layout());
    }

    VkPipelineLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layout_info.pSetLayouts = descriptor_set_layouts.data();
    layout_info.setLayoutCount = static_cast<std::uint32_t>(descriptor_set_layouts.size());
    vk_check(vkCreatePipelineLayout(m_context->get_device(), &layout_info, nullptr, &m_layout));
}

vk_pipeline_layout::~vk_pipeline_layout()
{
    vkDestroyPipelineLayout(m_context->get_device(), m_layout, nullptr);
}

vk_layout_manager::vk_layout_manager(vk_context* context) : m_context(context)
{
}

vk_layout_manager::~vk_layout_manager()
{
}

vk_parameter_layout* vk_layout_manager::get_parameter_layout(const rhi_parameter_desc& desc)
{
    std::uint64_t hash = hash::city_hash_64(&desc, sizeof(rhi_parameter_desc));
    auto iter = m_parameter_layouts.find(hash);
    if (iter != m_parameter_layouts.end())
        return iter->second.get();

    auto layout = std::make_unique<vk_parameter_layout>(desc, m_context);
    vk_parameter_layout* result = layout.get();
    m_parameter_layouts[hash] = std::move(layout);
    return result;
}

vk_pipeline_layout* vk_layout_manager::get_pipeline_layout(
    const rhi_parameter_desc* parameters,
    std::size_t parameter_count)
{
    std::uint64_t hash = 0;
    for (std::size_t i = 0; i < parameter_count; ++i)
        hash::combine(hash, hash::city_hash_64(&parameters[i], sizeof(rhi_parameter_desc)));

    auto iter = m_pipeline_layouts.find(hash);
    if (iter != m_pipeline_layouts.end())
        return iter->second.get();

    auto layout = std::make_unique<vk_pipeline_layout>(parameters, parameter_count, m_context);
    vk_pipeline_layout* result = layout.get();
    m_pipeline_layouts[hash] = std::move(layout);
    return result;
}

vk_parameter::vk_parameter(const rhi_parameter_desc& desc, vk_context* context) : m_context(context)
{
    m_layout = m_context->get_layout_manager()->get_parameter_layout(desc);

    auto& parameter_bindings = m_layout->get_parameter_bindings();
    for (auto& parameter_binding : parameter_bindings)
    {
        if (parameter_binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            rhi_buffer_desc uniform_buffer_desc = {};
            uniform_buffer_desc.size =
                parameter_binding.uniform_buffer.size * m_context->get_frame_resource_count();
            uniform_buffer_desc.flags = RHI_BUFFER_FLAG_UNIFORM | RHI_BUFFER_FLAG_HOST_VISIBLE;

            m_uniform_buffers.push_back(
                std::make_unique<vk_buffer>(uniform_buffer_desc, m_context));
        }
        else if (parameter_binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            m_images.push_back({});
        }
    }
    m_frame_resources.resize(m_context->get_frame_resource_count());
    for (auto& frame_resource : m_frame_resources)
        frame_resource.descriptor_update_count.resize(parameter_bindings.size());

    for (std::size_t i = 0; i < m_frame_resources.size(); ++i)
    {
        m_frame_resources[i].descriptor_set =
            m_context->allocate_descriptor_set(m_layout->get_layout());

        std::vector<VkWriteDescriptorSet> descriptor_write;
        std::vector<VkDescriptorBufferInfo> buffer_infos;
        buffer_infos.reserve(parameter_bindings.size());

        for (std::size_t j = 0; j < parameter_bindings.size(); ++j)
        {
            if (parameter_bindings[j].type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            {
                VkDescriptorBufferInfo info = {};
                info.buffer = m_uniform_buffers[parameter_bindings[j].index]->get_buffer_handle();
                info.offset = parameter_bindings[j].uniform_buffer.size * i;
                info.range = parameter_bindings[j].uniform_buffer.size;
                buffer_infos.push_back(info);

                VkWriteDescriptorSet write = {};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = m_frame_resources[i].descriptor_set;
                write.dstBinding = static_cast<std::uint32_t>(j);
                write.dstArrayElement = 0;
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.descriptorCount = 1;
                write.pBufferInfo = &buffer_infos.back();
                write.pImageInfo = nullptr;
                write.pTexelBufferView = nullptr;

                descriptor_write.push_back(write);
            }
        }

        if (!descriptor_write.empty())
        {
            vkUpdateDescriptorSets(
                m_context->get_device(),
                static_cast<std::uint32_t>(descriptor_write.size()),
                descriptor_write.data(),
                0,
                nullptr);
        }
    }
}

vk_parameter::~vk_parameter()
{
    for (frame_resource& frame_resource : m_frame_resources)
        m_context->free_descriptor_set(frame_resource.descriptor_set);
}

void vk_parameter::set_uniform(
    std::size_t index,
    const void* data,
    std::size_t size,
    std::size_t offset)
{
    sync();

    std::size_t current_index = m_context->get_frame_resource_index();

    auto& parameter_binding = m_layout->get_parameter_bindings()[index];
    assert(parameter_binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);

    void* buffer = m_uniform_buffers[parameter_binding.index]->get_buffer();
    void* target = static_cast<std::uint8_t*>(buffer) + offset +
                   parameter_binding.uniform_buffer.size * current_index;

    std::memcpy(target, data, size);

    mark_dirty(index);
}

void vk_parameter::set_texture(std::size_t index, rhi_texture* texture, rhi_sampler* sampler)
{
    sync();

    vk_texture* image = static_cast<vk_texture*>(texture);

    VkDescriptorImageInfo info = {};
    info.imageView = image->get_image_view();
    info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.sampler = static_cast<vk_sampler*>(sampler)->get_sampler();

    auto& binding = m_layout->get_parameter_bindings()[index];
    assert(binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    if (m_images[binding.index].first == info.imageView &&
        m_images[binding.index].second == info.sampler)
    {
        return;
    }
    else
    {
        m_images[binding.index].first = info.imageView;
        m_images[binding.index].second = info.sampler;
    }

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_frame_resources[m_context->get_frame_resource_index()].descriptor_set;
    write.dstBinding = static_cast<std::uint32_t>(index);
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pBufferInfo = nullptr;
    write.pImageInfo = &info;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_context->get_device(), 1, &write, 0, nullptr);

    mark_dirty(index);
}

void vk_parameter::set_storage(std::size_t index, rhi_buffer* storage_buffer)
{
    sync();

    vk_buffer* buffer = static_cast<vk_buffer*>(storage_buffer);

    VkDescriptorBufferInfo info = {};
    info.buffer = buffer->get_buffer_handle();
    info.offset = 0;
    info.range = buffer->get_buffer_size();

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = m_frame_resources[m_context->get_frame_resource_index()].descriptor_set;
    write.dstBinding = static_cast<std::uint32_t>(index);
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &info;
    write.pImageInfo = nullptr;
    write.pTexelBufferView = nullptr;

    vkUpdateDescriptorSets(m_context->get_device(), 1, &write, 0, nullptr);

    mark_dirty(index);
}

VkDescriptorSet vk_parameter::get_descriptor_set() const noexcept
{
    return m_frame_resources[m_context->get_frame_resource_index()].descriptor_set;
}

void vk_parameter::sync()
{
    std::size_t frame_resource_count = m_context->get_frame_resource_count();

    std::size_t current_index = m_context->get_frame_resource_index();
    std::size_t previous_index = (current_index + frame_resource_count - 1) % frame_resource_count;

    auto& current_frame_resource = m_frame_resources[current_index];
    auto& previous_frame_resource = m_frame_resources[previous_index];

    if (current_frame_resource.update_count >= previous_frame_resource.update_count)
        return;
    current_frame_resource.update_count = previous_frame_resource.update_count;

    std::vector<VkCopyDescriptorSet> descriptor_copy;

    for (std::size_t i = 0; i < current_frame_resource.descriptor_update_count.size(); ++i)
    {
        if (current_frame_resource.descriptor_update_count[i] >=
            previous_frame_resource.descriptor_update_count[i])
            continue;
        current_frame_resource.descriptor_update_count[i] =
            previous_frame_resource.descriptor_update_count[i];

        auto& parameter_binding = m_layout->get_parameter_bindings()[i];
        if (parameter_binding.type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
        {
            void* buffer = m_uniform_buffers[parameter_binding.index]->get_buffer();
            std::size_t size = parameter_binding.uniform_buffer.size;

            void* source = static_cast<std::uint8_t*>(buffer) + previous_index * size;
            void* target = static_cast<std::uint8_t*>(buffer) + current_index * size;

            std::memcpy(target, source, size);
        }
        else if (
            parameter_binding.type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
            parameter_binding.type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER)
        {
            VkCopyDescriptorSet copy = {};
            copy.sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET;
            copy.srcSet = previous_frame_resource.descriptor_set;
            copy.srcBinding = i;
            copy.srcArrayElement = 0;
            copy.dstSet = current_frame_resource.descriptor_set;
            copy.dstBinding = i;
            copy.dstArrayElement = 0;
            copy.descriptorCount = 1;
            descriptor_copy.push_back(copy);
        }
    }

    if (!descriptor_copy.empty())
    {
        vkUpdateDescriptorSets(
            m_context->get_device(),
            0,
            nullptr,
            static_cast<std::uint32_t>(descriptor_copy.size()),
            descriptor_copy.data());
    }
}

void vk_parameter::mark_dirty(std::size_t descriptor_index)
{
    std::size_t frame_resource_count = m_context->get_frame_resource_count();
    std::size_t current_index = m_context->get_frame_resource_index();
    std::size_t previous_index = (current_index + frame_resource_count - 1) % frame_resource_count;

    auto& current_frame_resource = m_frame_resources[current_index];
    auto& previous_frame_resource = m_frame_resources[previous_index];

    current_frame_resource.descriptor_update_count[descriptor_index] =
        previous_frame_resource.descriptor_update_count[descriptor_index] + 1;
    current_frame_resource.update_count = previous_frame_resource.update_count + 1;
}

vk_shader::vk_shader(const char* path, VkDevice device) : m_module(VK_NULL_HANDLE), m_device(device)
{
    std::ifstream fin(path, std::ios::binary);
    if (!fin.is_open())
        throw vk_exception("Failed to open file!");

    fin.seekg(0, std::ios::end);
    std::size_t spirv_size = fin.tellg();

    std::vector<char> spirv(spirv_size);
    fin.seekg(0);
    fin.read(spirv.data(), spirv_size);
    fin.close();

    VkShaderModuleCreateInfo shader_module_info = {};
    shader_module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_module_info.pCode = reinterpret_cast<std::uint32_t*>(spirv.data());
    shader_module_info.codeSize = spirv.size();

    vk_check(vkCreateShaderModule(device, &shader_module_info, nullptr, &m_module));
}

vk_shader::~vk_shader()
{
    vkDestroyShaderModule(m_device, m_module, nullptr);
}

vk_render_pipeline::vk_render_pipeline(
    const rhi_render_pipeline_desc& desc,
    VkExtent2D extent,
    vk_context* context)
    : m_pipeline(VK_NULL_HANDLE),
      m_pipeline_layout(VK_NULL_HANDLE),
      m_context(context)
{
    vk_shader* vertex_shader = static_cast<vk_shader*>(desc.vertex_shader);
    vk_shader* fragment_shader = static_cast<vk_shader*>(desc.fragment_shader);

    VkPipelineShaderStageCreateInfo vert_shader_stage_info = {};
    vert_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vert_shader_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vert_shader_stage_info.module = vertex_shader->get_module();
    vert_shader_stage_info.pName = "vs_main";

    VkPipelineShaderStageCreateInfo frag_shader_stage_info = {};
    frag_shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    frag_shader_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    frag_shader_stage_info.module = fragment_shader->get_module();
    frag_shader_stage_info.pName = "ps_main";

    VkPipelineShaderStageCreateInfo shader_stage_infos[] = {
        vert_shader_stage_info,
        frag_shader_stage_info};

    std::vector<VkVertexInputBindingDescription> binding_descriptions;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    for (std::size_t i = 0; i < desc.input.vertex_attribute_count; ++i)
    {
        VkFormat format = vk_util::map_format(desc.input.vertex_attributes[i].format);

        VkVertexInputBindingDescription binding = {};
        binding.binding = static_cast<std::uint32_t>(i);
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
        throw vk_exception("Invalid primitive topology.");
    }

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = extent.height;
    viewport.width = extent.width;
    viewport.height = -extent.height;
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
        throw vk_exception("Invalid cull mode.");
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
    depth_stencil_state_info.depthWriteEnable = VK_TRUE;
    switch (desc.depth_stencil.depth_functor)
    {
    case RHI_DEPTH_STENCIL_FUNCTOR_NEVER:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_NEVER;
        break;
    case RHI_DEPTH_STENCIL_FUNCTOR_LESS:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS;
        break;
    case RHI_DEPTH_STENCIL_FUNCTOR_EQUAL:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_EQUAL;
        break;
    case RHI_DEPTH_STENCIL_FUNCTOR_LESS_EQUAL:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        break;
    case RHI_DEPTH_STENCIL_FUNCTOR_GREATER:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_GREATER;
        break;
    case RHI_DEPTH_STENCIL_FUNCTOR_NOT_EQUAL:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_NOT_EQUAL;
        break;
    case RHI_DEPTH_STENCIL_FUNCTOR_GREATER_EQUAL:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_GREATER_OR_EQUAL;
        break;
    case RHI_DEPTH_STENCIL_FUNCTOR_ALWAYS:
        depth_stencil_state_info.depthCompareOp = VK_COMPARE_OP_ALWAYS;
        break;
    }
    depth_stencil_state_info.stencilTestEnable = VK_FALSE;
    depth_stencil_state_info.front = {};
    depth_stencil_state_info.back = {};

    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments(
        desc.blend.attachment_count);
    for (std::size_t i = 0; i < desc.blend.attachment_count; ++i)
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

    vk_layout_manager* layout_manager = m_context->get_layout_manager();
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    for (std::size_t i = 0; i < desc.parameter_count; ++i)
    {
        vk_parameter_layout* layout = layout_manager->get_parameter_layout(desc.parameters[i]);
        descriptor_set_layouts.push_back(layout->get_layout());
    }

    m_pipeline_layout = layout_manager->get_pipeline_layout(desc.parameters, desc.parameter_count);

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
    pipeline_info.layout = m_pipeline_layout->get_layout();
    pipeline_info.renderPass = static_cast<vk_render_pass*>(desc.render_pass)->get_render_pass();
    pipeline_info.subpass = static_cast<std::uint32_t>(desc.render_subpass_index);
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
    VkPipelineShaderStageCreateInfo shader_stage_info = {};
    shader_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_info.module = static_cast<vk_shader*>(desc.compute_shader)->get_module();
    shader_stage_info.pName = "cs_main";

    vk_layout_manager* layout_manager = m_context->get_layout_manager();
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    for (std::size_t i = 0; i < desc.parameter_count; ++i)
    {
        vk_parameter_layout* layout = layout_manager->get_parameter_layout(desc.parameters[i]);
        descriptor_set_layouts.push_back(layout->get_layout());
    }

    m_pipeline_layout = layout_manager->get_pipeline_layout(desc.parameters, desc.parameter_count);

    VkComputePipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;
    pipeline_info.stage = shader_stage_info;
    pipeline_info.layout = m_pipeline_layout->get_layout();

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
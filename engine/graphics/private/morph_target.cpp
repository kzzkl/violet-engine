#include "graphics/morph_target.hpp"
#include "math/math.hpp"
#include <limits>

namespace violet
{
morph_target_buffer::morph_target::morph_target(const std::vector<morph_element>& elements)
    : m_elements(elements)
{
    m_position_min = std::numeric_limits<float>::max();
    for (const auto& element : elements)
    {
        m_position_min = std::min(m_position_min, element.position.x);
        m_position_min = std::min(m_position_min, element.position.y);
        m_position_min = std::min(m_position_min, element.position.z);
    }
}

void morph_target_buffer::add_morph_target(const std::vector<morph_element>& elements)
{
    m_morph_targets.emplace_back(elements);
    m_dirty = true;
}

void morph_target_buffer::update_morph(
    rhi_command* command,
    rdg_allocator* allocator,
    rhi_buffer* morph_vertex_buffer,
    std::span<const float> weights)
{
    update_morph_data();

    rhi_buffer* weight_buffer = allocator->allocate_buffer({
        .size = weights.size() * sizeof(float),
        .flags = RHI_BUFFER_STORAGE | RHI_BUFFER_HOST_VISIBLE,
    });

    std::memcpy(
        weight_buffer->get_buffer_pointer(),
        weights.data(),
        weights.size() * sizeof(float));

    rhi_buffer_barrier barrier = {
        .buffer = morph_vertex_buffer,
        .src_access = RHI_ACCESS_SHADER_READ,
        .dst_access = RHI_ACCESS_TRANSFER_WRITE,
        .size = morph_vertex_buffer->get_buffer_size(),
    };

    command->set_pipeline_barrier(
        RHI_PIPELINE_STAGE_COMPUTE,
        RHI_PIPELINE_STAGE_TRANSFER,
        &barrier,
        1,
        nullptr,
        0);

    rhi_buffer_region region = {
        .size = morph_vertex_buffer->get_buffer_size(),
    };
    command->fill_buffer(morph_vertex_buffer, region, 0);

    barrier.src_access = RHI_ACCESS_TRANSFER_WRITE;
    barrier.dst_access = RHI_ACCESS_SHADER_WRITE;

    command->set_pipeline_barrier(
        RHI_PIPELINE_STAGE_TRANSFER,
        RHI_PIPELINE_STAGE_COMPUTE,
        &barrier,
        1,
        nullptr,
        0);

    morphing_cs::morphing_data data = {
        .morph_target_count = static_cast<std::uint32_t>(m_morph_targets.size()),
        .precision = m_precision,
        .header_buffer = m_header_buffer->get_handle(),
        .element_buffer = m_element_buffer->get_handle(),
        .morph_vertex_buffer = morph_vertex_buffer->get_handle(),
        .weight_buffer = weight_buffer->get_handle(),
    };
    rhi_parameter* parameter = allocator->allocate_parameter(morphing_cs::parameter);
    parameter->set_constant(0, &data, sizeof(morphing_cs::morphing_data));
    command->set_parameter(1, parameter);

    command->dispatch((m_morph_targets.size() + 7) / 8, (m_max_element_count + 7) / 8, 1);
}

void morph_target_buffer::update_morph_data()
{
    if (!m_dirty || m_morph_targets.empty())
    {
        return;
    }

    float precision_rcp = 1.0f / m_precision;

    struct morph_target_header
    {
        std::uint32_t element_count;
        std::uint32_t element_offset;
        std::int32_t position_min;
        std::uint32_t padding0;
    };
    std::vector<morph_target_header> morph_headers;
    morph_headers.reserve(m_morph_targets.size());

    struct morph_element_encode
    {
        vec3i position;
        std::uint32_t vertex_index;
    };
    std::vector<morph_element_encode> morph_elements;

    std::size_t element_count = 0;
    float position_min = std::numeric_limits<float>::max();
    for (auto& morph_target : m_morph_targets)
    {
        element_count += morph_target.get_elements().size();
        position_min = std::min(position_min, morph_target.get_position_min());
    }
    morph_elements.reserve(element_count);

    m_max_element_count = 0;
    std::uint32_t element_offset = 0;
    for (auto& morph_target : m_morph_targets)
    {
        const auto& elements = morph_target.get_elements();
        m_max_element_count = std::max(m_max_element_count, elements.size());

        morph_target_header morph_header = {
            .element_count = static_cast<std::uint32_t>(elements.size()),
            .element_offset = element_offset,
            .position_min = math::round<std::int32_t>(position_min * precision_rcp),
        };
        morph_headers.push_back(morph_header);

        for (const auto& element : elements)
        {
            morph_element_encode encode = {
                .position =
                    {
                        math::round<std::int32_t>(
                            (element.position.x - position_min) * precision_rcp),
                        math::round<std::int32_t>(
                            (element.position.y - position_min) * precision_rcp),
                        math::round<std::int32_t>(
                            (element.position.z - position_min) * precision_rcp),
                    },
                .vertex_index = element.vertex_index,
            };
            morph_elements.push_back(encode);
        }

        element_offset += morph_header.element_count;
    }

    auto& device = render_device::instance();

    m_header_buffer = device.create_buffer({
        .data = morph_headers.data(),
        .size = morph_headers.size() * sizeof(morph_target_header),
        .flags = RHI_BUFFER_STORAGE,
    });

    m_element_buffer = device.create_buffer({
        .data = morph_elements.data(),
        .size = morph_elements.size() * sizeof(morph_element_encode),
        .flags = RHI_BUFFER_STORAGE,
    });

    m_dirty = false;
}
} // namespace violet
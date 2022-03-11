#include "graphics.hpp"
#include "context.hpp"
#include "log.hpp"
#include "math.hpp"
#include "window.hpp"

using namespace ash::common;
using namespace ash::math;

namespace ash::graphics
{
graphics::graphics() : submodule("graphics")
{
}

bool graphics::initialize(const ash::common::dictionary& config)
{
    if (!m_plugin.load("ash-graphics-d3d12.dll") || !m_plugin.initialize(get_config(config)))
        return false;

    m_renderer = m_plugin.get_renderer();
    m_factory = m_plugin.get_factory();

    adapter_info info[4] = {};
    std::size_t num_adapter = m_renderer->get_adapter_info(info, 4);
    for (std::size_t i = 0; i < num_adapter; ++i)
    {
        log::debug("graphics adapter: {}", info[i].description);
    }

    auto& task = get_context().get_task();

    auto root_task = task.find("root");
    auto render_task = task.schedule("render", [this]() {
        m_renderer->begin_frame();
        render();
        m_renderer->end_frame();
    });
    render_task->add_dependency(*root_task);

    initialize_resource();

    return true;
}

void graphics::initialize_resource()
{
    {
        std::array<pipeline_parameter_part, 1> parameter_part = {
            pipeline_parameter_part{"object", pipeline_parameter_type::BUFFER}};
        pipeline_parameter_desc parameter_desc;
        parameter_desc.data = parameter_part.data();
        parameter_desc.size = parameter_part.size();

        float4x4 mvp = {-1.02709162,
                        0.00000000,
                        2.05418324,
                        0.00000000,
                        -2.22536516,
                        2.78170633,
                        -1.11268258,
                        -4.44895107e-07,
                        -0.666673362,
                        -0.666673362,
                        -0.333336681,
                        14.9901514,
                        -0.666666687,
                        -0.666666687,
                        -0.333333343,
                        15.0000010};

        m_mvp = m_factory->make_upload_buffer(256);
        m_mvp->upload(&mvp, sizeof(mvp));

        m_parameter = m_factory->make_pipeline_parameter(parameter_desc);
        m_parameter->bind(0, m_mvp);

        pipeline_parameter_layout_desc parameter_layout_desc = {};
        // desc.parameter_layout.data = &package;
        parameter_layout_desc.size = 1;
        parameter_layout_desc.data = &parameter_desc;
        m_layout = m_factory->make_pipeline_parameter_layout(parameter_layout_desc);
    }

    {
        pipeline_desc desc = {};
        desc.name = "test";
        std::array<vertex_attribute_desc, 2> attribute = {
            vertex_attribute_desc{"POSITION", vertex_attribute_type::FLOAT3, 0},
            vertex_attribute_desc{"COLOR", vertex_attribute_type::FLOAT4, 0}};
        desc.vertex_layout.data = attribute.data();
        desc.vertex_layout.size = attribute.size();
        desc.parameter_layout = m_layout;
        desc.vertex_shader = "resource/shader/base.hlsl";
        desc.pixel_shader = "resource/shader/base.hlsl";
        m_pipeline = m_factory->make_pipeline(desc);
    }

    {
        struct vertex
        {
            float3 position;
            float4 color;
        };

        std::vector<vertex> vertices = {
            vertex({float3{-1.0f, -1.0f, -1.0f},
                    float4{1.000000000f, 1.000000000f, 1.000000000f, 1.000000000f}}),
            vertex({float3{-1.0f, +1.0f, -1.0f},
                    float4{1.000000000f, 0.980392218f, 0.980392218f, 1.000000000f}}),
            vertex({float3{+1.0f, +1.0f, -1.0f},
                    float4{0.854902029f, 0.439215720f, 0.839215755f, 1.000000000f}}),
            vertex({float3{+1.0f, -1.0f, -1.0f},
                    float4{0.196078449f, 0.803921640f, 0.196078449f, 1.000000000f}}),
            vertex({float3{-1.0f, -1.0f, +1.0f},
                    float4{0.941176534f, 1.000000000f, 0.941176534f, 1.000000000f}}),
            vertex({float3{-1.0f, +1.0f, +1.0f},
                    float4{0.545098066f, 0.000000000f, 0.000000000f, 1.000000000f}}),
            vertex({float3{+1.0f, +1.0f, +1.0f},
                    float4{1.000000000f, 0.921568692f, 0.803921640f, 1.000000000f}}),
            vertex({float3{+1.0f, -1.0f, +1.0f},
                    float4{0.980392218f, 0.921568692f, 0.843137324f, 1.000000000f}})};

        std::vector<uint16_t> indices = {0, 1, 2, 0, 2, 3, 4, 6, 5, 4, 7, 6, 4, 5, 1, 4, 1, 0,
                                         3, 2, 6, 3, 6, 7, 1, 5, 6, 1, 6, 2, 4, 0, 3, 4, 3, 7};

        vertex_buffer_desc vertex_desc = {vertices.data(), sizeof(vertex), vertices.size()};
        index_buffer_desc index_desc = {indices.data(), sizeof(uint16_t), indices.size()};

        m_vertices = m_factory->make_vertex_buffer(vertex_desc);
        m_indices = m_factory->make_index_buffer(index_desc);
    }
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    command->set_pipeline(m_pipeline);
    command->set_layout(m_layout);
    command->set_parameter(0, m_parameter);
    command->draw(
        m_vertices,
        m_indices,
        primitive_topology_type::TRIANGLE_LIST,
        m_renderer->get_back_buffer());
    m_renderer->execute(command);
}

context_config graphics::get_config(const ash::common::dictionary& config)
{
    context_config result = {};

    auto& window = get_context().get_submodule<ash::window::window>();

    result.window_handle = window.get_handle();

    window::window_rect rect = window.get_rect();
    result.width = rect.width;
    result.height = rect.height;

    result.render_concurrency = 4;

    return result;
}
} // namespace ash::graphics
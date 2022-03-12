#include "graphics.hpp"
#include "config_parser.hpp"
#include "context.hpp"
#include "log.hpp"
#include "math.hpp"
#include "window.hpp"

using namespace ash::math;

namespace ash::graphics
{
graphics::graphics() noexcept : submodule("graphics")
{
}

bool graphics::initialize(const dictionary& config)
{
    m_config.load(config);

    auto& window = get_submodule<ash::window::window>();

    context_config desc = {};
    desc.window_handle = window.get_handle();
    window::window_rect rect = window.get_rect();
    desc.width = rect.width;
    desc.height = rect.height;
    desc.render_concurrency = m_config.get_render_concurrency();

    if (!m_plugin.load(m_config.get_plugin()) || !m_plugin.initialize(desc))
        return false;

    m_renderer = m_plugin.get_renderer();
    m_factory = m_plugin.get_factory();

    adapter_info info[4] = {};
    std::size_t num_adapter = m_renderer->get_adapter_info(info, 4);
    for (std::size_t i = 0; i < num_adapter; ++i)
    {
        log::debug("graphics adapter: {}", info[i].description);
    }

    initialize_resource();

    auto& task = get_submodule<task::task_manager>();

    auto root_task = task.find("root");
    auto render_task = task.schedule("render", [this]() {
        m_renderer->begin_frame();
        render();
        m_renderer->end_frame();
    });
    render_task->add_dependency(*root_task);

    return true;
}

bool graphics::initialize_resource()
{
    {
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

        {
            auto [found, desc] = m_config.find_desc<pipeline_parameter_desc>("object");
            m_parameter_object = m_factory->make_pipeline_parameter(desc);
            m_parameter_object->bind(0, m_mvp);
        }

        struct material
        {
            float4 color;
            float4 color2;
        };
        material m = {{0.5f, 0.5f, 0.5f, 1.0f}, {0.5f, 0.0f, 0.5f, 1.0f}};
        m_material = m_factory->make_upload_buffer(256);
        m_material->upload(&m, sizeof(m));

        {
            auto [found, desc] = m_config.find_desc<pipeline_parameter_desc>("material");
            m_parameter_material = m_factory->make_pipeline_parameter(desc);
            m_parameter_material->bind(0, m_material);
        }

        {
            auto [found, desc] = m_config.find_desc<pipeline_parameter_layout_desc>("base layout");
            m_layout = m_factory->make_pipeline_parameter_layout(desc);
        }
    }

    {
        auto [found, desc] = m_config.find_desc<pipeline_desc>("pass 1");
        desc.parameter_layout = m_layout;
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

    return true;
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    command->set_pipeline(m_pipeline);
    command->set_layout(m_layout);
    command->set_parameter(0, m_parameter_object);
    command->set_parameter(1, m_parameter_material);
    command->draw(
        m_vertices,
        m_indices,
        primitive_topology_type::TRIANGLE_LIST,
        m_renderer->get_back_buffer());
    m_renderer->execute(command);
}
} // namespace ash::graphics
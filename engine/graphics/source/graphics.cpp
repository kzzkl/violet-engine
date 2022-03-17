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
        m_view->each([](visual& visual, mesh& mesh, material& material) {
            if (visual.group != nullptr)
                visual.group->add(&mesh);
        });

        m_renderer->begin_frame();
        render();
        m_renderer->end_frame();

        for (auto& [name, group] : m_render_group)
            group->clear();
    });
    render_task->add_dependency(*root_task);

    auto& world = get_submodule<ecs::ecs>();
    world.register_component<visual, mesh, material>();
    m_view = world.create_view<visual, mesh, material>();

    return true;
}

render_group* graphics::get_group(std::string_view name)
{
    auto iter = m_render_group.find(name.data());
    if (iter == m_render_group.end())
        return make_render_group(name);
    else
        return iter->second.get();
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
            auto [found, desc, _] = m_config.find_desc<pipeline_parameter_desc>("object");
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
            auto [found, desc, _] = m_config.find_desc<pipeline_parameter_desc>("material");
            m_parameter_material = m_factory->make_pipeline_parameter(desc);
            m_parameter_material->bind(0, m_material);
        }
    }

    return true;
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    for (auto& [name, group] : m_render_group)
    {
        command->set_pipeline(group->get_pipeline());
        command->set_layout(group->get_layout());
        command->set_parameter(0, m_parameter_object);

        for (mesh* m : *group)
        {
            command->draw(
                m->vertex_buffer.get(),
                m->index_buffer.get(),
                primitive_topology_type::TRIANGLE_LIST,
                m_renderer->get_back_buffer());
        }
    }
    m_renderer->execute(command);
}

render_group* graphics::make_render_group(std::string_view name)
{
    auto [found, desc, config] = m_config.find_desc<pipeline_desc>(name);
    if (!found)
        return nullptr;

    // make layout
    auto [layout_found, layout_desc, _] =
        m_config.find_desc<pipeline_layout_desc>(config->parameter_layout);
    if (!layout_found)
        return nullptr;

    pipeline_layout* layout = m_factory->make_pipeline_layout(layout_desc);
    desc.layout = layout;

    m_render_group[name.data()] =
        std::make_unique<render_group>(layout, m_factory->make_pipeline(desc));
    return m_render_group[name.data()].get();
}
} // namespace ash::graphics
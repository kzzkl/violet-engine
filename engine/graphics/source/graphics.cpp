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
    auto scene_task = task.find("scene");
    auto render_task = task.schedule("render", [this]() {
        m_view->each([](visual& visual, mesh& mesh, material& material) {
            if (visual.group != nullptr)
                visual.group->add(render_unit{&mesh, visual.parameter.get()});
        });

        update_pass_data();

        m_renderer->begin_frame();
        render();
        m_renderer->end_frame();

        for (auto& [name, group] : m_render_group)
            group->clear();
    });
    render_task->add_dependency(*scene_task);

    auto& world = get_submodule<ecs::world>();
    world.register_component<visual, mesh, material, main_camera, camera>();
    m_view = world.make_view<visual, mesh, material>();
    m_camera_view = world.make_view<main_camera, camera, scene::transform>();

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
    /*float4x4 mvp = {-1.02709162,
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
    render_object_data rd;
    rd.to_world = mvp;
    m_object->set_data<0>(rd);*/

    m_parameter_pass = make_render_parameter<render_pass_data>("ash_pass");

    return true;
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    for (auto& [name, group] : m_render_group)
    {
        command->set_pipeline(group->get_pipeline());
        command->set_layout(group->get_layout());
        command->set_parameter(0, m_parameter_pass->get_parameter());

        for (auto [mesh, parameter] : *group)
        {
            // command->set_parameter(1, parameter->get_parameter());
            command->draw(
                mesh->vertex_buffer.get(),
                mesh->index_buffer.get(),
                primitive_topology_type::TRIANGLE_LIST,
                m_renderer->get_back_buffer());
        }
    }
    m_renderer->execute(command);
}

void graphics::update_pass_data()
{
    m_camera_view->each([this](main_camera&, camera& camera, scene::transform& transform) {
        if (transform.node->updated)
        {
            math::float4x4_simd to_world = math::simd::load(transform.node->to_world);
            math::float4x4_simd view = math::matrix_simd::inverse(to_world);
            math::float4x4_simd proj = math::simd::load(camera.get_perspective());

            render_pass_data data = {};
            math::simd::store(math::matrix_simd::transpose(view), data.camera_view);
            math::simd::store(math::matrix_simd::transpose(proj), data.camera_projection);
            math::simd::store(
                math::matrix_simd::transpose(math::matrix_simd::mul(view, proj)),
                data.camera_view_projection);

            m_parameter_pass->set_data<0>(data);
        }
    });

    m_parameter_pass->sync_resource();
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
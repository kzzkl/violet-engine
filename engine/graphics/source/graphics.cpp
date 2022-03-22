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

    auto& window = module<ash::window::window>();

    context_config desc = {};
    desc.window_handle = window.handle();
    window::window_rect rect = window.rect();
    desc.width = rect.width;
    desc.height = rect.height;
    desc.render_concurrency = m_config.render_concurrency();

    if (!m_plugin.load(m_config.plugin()) || !m_plugin.initialize(desc))
        return false;

    m_renderer = m_plugin.renderer();
    m_factory = m_plugin.factory();

    adapter_info info[4] = {};
    std::size_t num_adapter = m_renderer->adapter(info, 4);
    for (std::size_t i = 0; i < num_adapter; ++i)
    {
        log::debug("graphics adapter: {}", info[i].description);
    }

    initialize_resource();

    auto& task = module<task::task_manager>();
    auto scene_task = task.find("scene");
    auto render_task = task.schedule("render", [this]() {
        m_view->each([](visual& visual, mesh& mesh) {
            if (visual.group != nullptr)
            {
                visual.object->sync_resource();
                visual.material->sync_resource();
                visual.group->add(
                    render_unit{&mesh, visual.object->parameter(), visual.material->parameter()});
            }
        });

        update_pass_data();

        m_renderer->begin_frame();
        render();
        m_renderer->end_frame();

        for (auto& [name, group] : m_render_group)
            group->clear();
    });
    render_task->add_dependency(*scene_task);

    auto& world = module<ecs::world>();
    world.register_component<visual, mesh, main_camera, camera>();
    m_view = world.make_view<visual, mesh>();
    m_camera_view = world.make_view<main_camera, camera, scene::transform>();

    return true;
}

render_group* graphics::group(std::string_view name)
{
    auto iter = m_render_group.find(name.data());
    if (iter == m_render_group.end())
        return make_render_group(name);
    else
        return iter->second.get();
}

bool graphics::initialize_resource()
{
    m_parameter_pass = make_render_parameter<multiple<render_pass_data>>("ash_pass");

    return true;
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    for (auto& [name, group] : m_render_group)
    {
        command->pipeline(group->pipeline());
        command->layout(group->layout());
        command->parameter(2, m_parameter_pass->parameter());

        for (auto [mesh, object, material] : *group)
        {
            command->parameter(0, object);
            command->parameter(1, material);

            command->draw(
                mesh->vertex_buffer.get(),
                mesh->index_buffer.get(),
                primitive_topology_type::TRIANGLE_LIST,
                m_renderer->back_buffer());
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
            math::float4x4_simd proj = math::simd::load(camera.perspective());

            render_pass_data data = {};
            math::simd::store(math::matrix_simd::transpose(view), data.camera_view);
            math::simd::store(math::matrix_simd::transpose(proj), data.camera_projection);
            math::simd::store(
                math::matrix_simd::transpose(math::matrix_simd::mul(view, proj)),
                data.camera_view_projection);

            m_parameter_pass->set<0>(data);
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
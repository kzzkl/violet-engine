#include "graphics.hpp"
#include "config_parser.hpp"
#include "context.hpp"
#include "log.hpp"
#include "math.hpp"
#include "window.hpp"
#include <fstream>

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
        m_view->each([](visual& visual, scene::transform& transform) {
            visual.object->set(0, transform.node->to_parent);
            for (std::size_t i = 0; i < visual.submesh.size(); ++i)
            {
                if (visual.material[i].pipeline == nullptr)
                    continue;
                visual.material[i].pipeline->add(render_unit{
                    visual.vertex_buffer.get(),
                    visual.index_buffer.get(),
                    visual.submesh[i].index_start,
                    visual.submesh[i].index_end,
                    visual.object->parameter(),
                    visual.material[i].property->parameter()});
            }
        });

        update_pass_data();

        m_renderer->begin_frame();
        render();
        m_renderer->end_frame();

        for (auto& [name, group] : m_render_pipeline)
            group->clear();
    });
    render_task->add_dependency(*scene_task);

    auto& world = module<ecs::world>();
    world.register_component<visual, main_camera, camera>();
    m_view = world.make_view<visual, scene::transform>();
    m_camera_view = world.make_view<main_camera, camera, scene::transform>();

    return true;
}

render_pipeline* graphics::make_render_pipeline(std::string_view name)
{
    auto iter = m_render_pipeline.find(name.data());
    if (iter != m_render_pipeline.end())
        return iter->second.get();

    auto [found, desc, config] = m_config.find_desc<pipeline_desc>(name);
    if (!found)
        return nullptr;

    // make layout
    pipeline_layout_desc layout_desc = {};
    layout_desc.size = 3;

    {
        auto [object_fount, object_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>("ash_object");
        if (!object_fount)
        {
            log::error("object layout no found: ash_object");
            return nullptr;
        }
        layout_desc.data[0] = object_desc;
    }

    {
        auto [material_found, material_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(config->material_layout);
        if (!material_found)
        {
            log::error("material no found: {}", config->material_layout);
            return nullptr;
        }
        layout_desc.data[1] = material_desc;
    }

    {
        auto [pass_found, pass_desc, _] = m_config.find_desc<pipeline_parameter_desc>("ash_pass");
        if (!pass_found)
        {
            log::error("pass layout no found: ash_pass");
            return nullptr;
        }
        layout_desc.data[2] = pass_desc;
    }

    pipeline_layout* layout = m_factory->make_pipeline_layout(layout_desc);
    desc.layout = layout;

    auto pipeline = std::make_unique<render_pipeline>(layout, m_factory->make_pipeline(desc));

    m_render_pipeline[name.data()] = std::move(pipeline);
    return m_render_pipeline[name.data()].get();
}

std::unique_ptr<resource> graphics::make_texture(std::string_view file)
{
    std::ifstream fin(file.data(), std::ios::in | std::ios::binary);
    if (!fin)
    {
        log::error("Can not open texture: {}.", file);
        nullptr;
    }

    std::vector<uint8_t> dds_data(fin.seekg(0, std::ios::end).tellg());
    fin.seekg(0, std::ios::beg).read((char*)dds_data.data(), dds_data.size());
    fin.close();

    return std::unique_ptr<resource>(m_factory->make_texture(dds_data.data(), dds_data.size()));
}

bool graphics::initialize_resource()
{
    m_parameter_pass = make_render_parameter("ash_pass");
    return true;
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    for (auto& [name, pipeline] : m_render_pipeline)
    {
        command->pipeline(pipeline->pipeline());
        command->layout(pipeline->layout());

        command->parameter(2, m_parameter_pass->parameter());

        for (auto& unit : pipeline->units())
        {
            command->parameter(0, unit.object);
            command->parameter(1, unit.material);

            command->draw(
                unit.vertex_buffer,
                unit.index_buffer,
                unit.index_start,
                unit.index_end,
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
            math::float4x4_simd world_simd = math::simd::load(transform.node->to_world);
            math::float4x4_simd view_simd = math::matrix_simd::inverse(world_simd);
            math::float4x4_simd projection_simd = math::simd::load(camera.perspective());

            math::float4x4 view, projection, view_projection;
            math::simd::store(math::matrix_simd::transpose(view_simd), view);
            math::simd::store(math::matrix_simd::transpose(projection_simd), projection);
            math::simd::store(
                math::matrix_simd::transpose(math::matrix_simd::mul(view_simd, projection_simd)),
                view_projection);

            m_parameter_pass->set(0, float4{1.0f, 2.0f, 3.0f, 4.0f});
            m_parameter_pass->set(1, float4{5.0f, 6.0f, 7.0f, 8.0f});
            m_parameter_pass->set(2, view, false);
            m_parameter_pass->set(3, projection, false);
            m_parameter_pass->set(4, view_projection, false);
        }
    });
}
} // namespace ash::graphics
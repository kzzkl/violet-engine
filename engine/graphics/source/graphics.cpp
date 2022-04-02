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
    desc.multiple_sampling = m_config.multiple_sampling();
    desc.frame_resource = m_config.frame_resource();

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
    initialize_debug();

    auto& task = module<task::task_manager>();
    task.schedule(TASK_RENDER, [this]() {
        update();

        if (!m_debug->empty())
            update_debug();

        m_renderer->begin_frame();
        render();

        if (!m_debug->empty())
            render_debug();

        m_renderer->end_frame();

        for (auto pipeline : m_render_pipelines)
            pipeline->clear();
        m_render_pipelines.clear();
    });

    auto& world = module<ecs::world>();
    world.register_component<visual, main_camera, camera>();
    m_object_view = world.make_view<visual, scene::transform>();
    m_camera_view = world.make_view<main_camera, camera, scene::transform>();

    return true;
}

std::unique_ptr<render_pipeline> graphics::make_render_pipeline(std::string_view name)
{
    auto [found, desc, config] = m_config.find_desc<pipeline_desc>(name);
    if (!found)
        return nullptr;

    // make layout
    pipeline_layout_desc layout_desc = {};
    layout_desc.size = 0;

    if (config->object_parameter != "none")
    {
        ASH_ASSERT(config->object_parameter == "ash_object");

        auto [object_fount, object_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(config->object_parameter);
        if (!object_fount)
        {
            log::error("object layout no found: {}", config->object_parameter);
            return nullptr;
        }
        layout_desc.data[layout_desc.size] = object_desc;
        ++layout_desc.size;
    }

    if (config->material_parameter != "none")
    {
        auto [material_found, material_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(config->material_parameter);
        if (!material_found)
        {
            log::error("material no found: {}", config->material_parameter);
            return nullptr;
        }
        layout_desc.data[layout_desc.size] = material_desc;
        ++layout_desc.size;
    }

    if (config->pass_parameter != "none")
    {
        ASH_ASSERT(config->pass_parameter == "ash_pass");

        auto [pass_found, pass_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(config->pass_parameter);
        if (!pass_found)
        {
            log::error("pass layout no found: {}", config->pass_parameter);
            return nullptr;
        }
        layout_desc.data[layout_desc.size] = pass_desc;
        ++layout_desc.size;
    }

    pipeline_layout* layout = m_factory->make_pipeline_layout(layout_desc);
    desc.layout = layout;

    return std::make_unique<render_pipeline>(layout, m_factory->make_pipeline(desc));
}

std::unique_ptr<resource> graphics::make_texture(std::string_view file)
{
    std::ifstream fin(file.data(), std::ios::in | std::ios::binary);
    if (!fin)
    {
        log::error("Can not open texture: {}.", file);
        return nullptr;
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

void graphics::initialize_debug()
{
    vertex_buffer_desc vertex_desc = {};
    vertex_desc.dynamic = true;
    vertex_desc.vertex_size = sizeof(graphics_debug::vertex);
    vertex_desc.vertex_count = 2048;
    vertex_desc.vertices = nullptr;

    std::vector<resource*> vertex_buffer(m_config.frame_resource());

    for (auto& buffer : vertex_buffer)
        buffer = make_vertex_buffer<graphics_debug::vertex>(nullptr, 2048, true).release();

    std::vector<std::uint16_t> index_data(4096);
    for (std::uint16_t i = 0; i < 4096; ++i)
        index_data[i] = i;

    resource* index_buffer = make_index_buffer(index_data.data(), index_data.size()).release();

    m_debug = std::make_unique<graphics_debug>(vertex_buffer, index_buffer);
    m_debug_pipeline = make_render_pipeline("debug");
}

void graphics::update()
{
    math::float4x4_simd transform_v;
    math::float4x4_simd transform_p;
    math::float4x4_simd transform_vp;

    m_camera_view->each([&, this](main_camera&, camera& camera, scene::transform& transform) {
        if (transform.node()->sync_count() != 0)
        {
            math::float4x4_simd world_simd = math::simd::load(transform.node()->to_world);
            transform_v = math::matrix_simd::inverse(world_simd);
            math::simd::store(transform_v, camera.view);
        }
        else
        {
            transform_v = math::simd::load(camera.view);
        }

        transform_p = math::simd::load(camera.projection);
        transform_vp = math::matrix_simd::mul(transform_v, transform_p);

        if (transform.node()->sync_count() != 0)
        {
            math::float4x4 view, projection, view_projection;
            math::simd::store(math::matrix_simd::transpose(transform_v), view);
            math::simd::store(math::matrix_simd::transpose(transform_p), projection);
            math::simd::store(math::matrix_simd::transpose(transform_vp), view_projection);

            m_parameter_pass->set(0, float4{1.0f, 2.0f, 3.0f, 4.0f});
            m_parameter_pass->set(1, float4{5.0f, 6.0f, 7.0f, 8.0f});
            m_parameter_pass->set(2, view, false);
            m_parameter_pass->set(3, projection, false);
            m_parameter_pass->set(4, view_projection, false);
        }
    });

    m_object_view->each([&, this](visual& visual, scene::transform& transform) {
        if (!transform.node()->in_view)
            return;

        math::float4x4_simd transform_m = math::simd::load(transform.node()->to_world);
        math::float4x4_simd transform_mv = math::matrix_simd::mul(transform_m, transform_v);
        math::float4x4_simd transform_mvp = math::matrix_simd::mul(transform_mv, transform_p);

        math::float4x4 model, model_view, model_view_projection;
        math::simd::store(math::matrix_simd::transpose(transform_m), model);
        math::simd::store(math::matrix_simd::transpose(transform_mv), model_view);
        math::simd::store(math::matrix_simd::transpose(transform_mvp), model_view_projection);

        visual.object->set(0, model, false);
        visual.object->set(1, model_view, false);
        visual.object->set(2, model_view_projection, false);

        for (std::size_t i = 0; i < visual.submesh.size(); ++i)
        {
            if (visual.material[i].pipeline == nullptr)
                continue;
            m_render_pipelines.insert(visual.material[i].pipeline);
            visual.material[i].pipeline->add(render_unit{
                visual.vertex_buffer.get(),
                visual.index_buffer.get(),
                visual.submesh[i].index_start,
                visual.submesh[i].index_end,
                visual.object->parameter(),
                visual.material[i].property->parameter()});
        }
    });
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    for (auto pipeline : m_render_pipelines)
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

void graphics::update_debug()
{
    m_debug->sync();
}

void graphics::render_debug()
{
    auto command = m_renderer->allocate_command();

    command->pipeline(m_debug_pipeline->pipeline());
    command->layout(m_debug_pipeline->layout());
    command->parameter(0, m_parameter_pass->parameter());
    command->draw(
        m_debug->vertex_buffer(),
        m_debug->index_buffer(),
        0,
        m_debug->vertex_count() * 2,
        primitive_topology_type::LINE_LIST,
        m_renderer->back_buffer());

    m_renderer->execute(command);

    m_debug->clear();
}
} // namespace ash::graphics
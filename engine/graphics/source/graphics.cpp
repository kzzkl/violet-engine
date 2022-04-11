#include "graphics.hpp"
#include "context.hpp"
#include "graphics_config.hpp"
#include "log.hpp"
#include "math.hpp"
#include "window.hpp"
#include <fstream>

using namespace ash::math;

namespace ash::graphics
{
graphics::graphics() noexcept : system_base("graphics")
{
}

bool graphics::initialize(const dictionary& config)
{
    m_config.load(config);

    auto& window = system<ash::window::window>();

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

    auto& task = system<task::task_manager>();
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

    auto& world = system<ecs::world>();
    world.register_component<visual>();
    world.register_component<main_camera>();
    world.register_component<camera>();
    m_object_view = world.make_view<visual, scene::transform>();
    m_camera_view = world.make_view<main_camera, camera, scene::transform>();

    return true;
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

std::tuple<bool, std::size_t, std::size_t> graphics::make_pipeline(
    std::string_view name,
    pipeline_layout*& layout,
    pipeline*& pipeline)
{
    auto [found, desc, config] = m_config.find_desc<pipeline_desc>(name);
    if (!found)
        return {false, 0, 0};

    // make layout
    pipeline_layout_desc layout_desc = {};
    layout_desc.size = 0;

    for (auto& parameter : config->unit_parameters)
    {
        auto [parameter_found, parameter_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(parameter);
        if (!parameter_found)
        {
            log::error("unit parameter no found: {}", parameter);
            return {false, 0, 0};
        }
        layout_desc.data[layout_desc.size] = parameter_desc;
        ++layout_desc.size;
    }

    for (auto& parameter : config->pass_parameters)
    {
        auto [parameter_found, parameter_desc, _] =
            m_config.find_desc<pipeline_parameter_desc>(parameter);
        if (!parameter_found)
        {
            log::error("pass parameter no found: {}", parameter);
            return {false, 0, 0};
        }
        layout_desc.data[layout_desc.size] = parameter_desc;
        ++layout_desc.size;
    }

    layout = m_factory->make_pipeline_layout(layout_desc);
    desc.layout = layout;
    pipeline = m_factory->make_pipeline(desc);

    return {true, config->unit_parameters.size(), config->pass_parameters.size()};
}

bool graphics::initialize_resource()
{
    m_parameter_pass = make_render_parameter("ash_pass");
    return true;
}

void graphics::initialize_debug()
{
    static constexpr std::size_t MAX_VERTEX_COUNT = 4096 * 16;

    vertex_buffer_desc vertex_desc = {};
    vertex_desc.dynamic = true;
    vertex_desc.vertex_size = sizeof(debug_pipeline::vertex);
    vertex_desc.vertex_count = MAX_VERTEX_COUNT;
    vertex_desc.vertices = nullptr;

    std::vector<resource*> vertex_buffer(m_config.frame_resource());

    for (auto& buffer : vertex_buffer)
        buffer =
            make_vertex_buffer<debug_pipeline::vertex>(nullptr, MAX_VERTEX_COUNT, true).release();

    std::vector<std::uint32_t> index_data(MAX_VERTEX_COUNT * 2);
    for (std::uint32_t i = 0; i < MAX_VERTEX_COUNT * 2; ++i)
        index_data[i] = i;

    resource* index_buffer = make_index_buffer(index_data.data(), index_data.size()).release();

    m_debug = make_render_pipeline<debug_pipeline>("debug", vertex_buffer, index_buffer);
}

void graphics::update()
{
    math::float4x4_simd transform_v;
    math::float4x4_simd transform_p;
    math::float4x4_simd transform_vp;

    m_camera_view->each([&, this](main_camera&, camera& camera, scene::transform& transform) {
        if (transform.node->sync_count != 0)
        {
            math::float4x4_simd world_simd = math::simd::load(transform.world_matrix);
            transform_v = math::matrix_simd::inverse(world_simd);
            math::simd::store(transform_v, camera.view);
        }
        else
        {
            transform_v = math::simd::load(camera.view);
        }

        transform_p = math::simd::load(camera.projection);
        transform_vp = math::matrix_simd::mul(transform_v, transform_p);

        if (transform.node->sync_count != 0)
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
        math::float4x4_simd transform_m = math::simd::load(transform.world_matrix);
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
            if (visual.submesh[i].pipeline == nullptr)
                continue;
            m_render_pipelines.insert(visual.submesh[i].pipeline);
            visual.submesh[i].pipeline->add(&visual.submesh[i]);
        }
    });
}

void graphics::render()
{
    auto command = m_renderer->allocate_command();
    for (auto pipeline : m_render_pipelines)
    {
        pipeline->render(m_renderer->back_buffer(), command, m_parameter_pass.get());
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
    m_debug->render(m_renderer->back_buffer(), command, m_parameter_pass.get());
    m_renderer->execute(command);
    m_debug->reset();
}
} // namespace ash::graphics
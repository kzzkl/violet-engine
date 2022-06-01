#include "ui/ui.hpp"
#include "core/relation.hpp"
#include "graphics/graphics.hpp"
#include "graphics/graphics_task.hpp"
#include "ui/ui_event.hpp"
#include "ui/ui_task.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"

namespace ash::ui
{
static constexpr std::size_t MAX_UI_VERTEX_COUNT = 4096 * 16;
static constexpr std::size_t MAX_UI_INDEX_COUNT = MAX_UI_VERTEX_COUNT * 2;

ui::ui() : system_base("ui"), m_material_parameter_counter(0)
{
}

bool ui::initialize(const dictionary& config)
{
    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    m_font = std::make_unique<font_type>("engine/font/Roboto-Regular.ttf", 13);
    m_tree = std::make_unique<element_tree>();

    m_pipeline = std::make_unique<ui_pipeline>();
    m_mvp_parameter = graphics.make_pipeline_parameter("ui_mvp");

    m_vertex_buffers.push_back(graphics.make_vertex_buffer<math::float3>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true));
    m_vertex_buffers.push_back(graphics.make_vertex_buffer<math::float2>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true));
    m_vertex_buffers.push_back(graphics.make_vertex_buffer<std::uint32_t>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true));
    m_index_buffer = graphics.make_index_buffer<std::uint32_t>(nullptr, MAX_UI_INDEX_COUNT, true);

    m_entity = world.create("ui root");
    world.add<graphics::visual>(m_entity);

    auto& visual = world.component<graphics::visual>(m_entity);
    visual.groups = graphics::VISUAL_GROUP_UI;
    for (auto& vertex_buffer : m_vertex_buffers)
        visual.vertex_buffers.push_back(vertex_buffer.get());
    visual.index_buffer = m_index_buffer.get();

    event.register_event<event_calculate_layout>();

    event.subscribe<window::event_window_resize>(
        "ui",
        [this](std::uint32_t width, std::uint32_t height) { resize(width, height); });

    auto window_extent = system<window::window>().extent();
    resize(window_extent.width, window_extent.height);

    auto& task = system<task::task_manager>();
    auto ui_tick_task = task.schedule(TASK_UI_TICK, [this]() { tick(); });
    ui_tick_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_END));

    auto render_task = task.find(graphics::TASK_GRAPHICS_RENDER);
    render_task->add_dependency(*ui_tick_task);

    return true;
}

void ui::tick()
{
    auto& world = system<ecs::world>();

    m_renderer.reset();

    auto extent = system<window::window>().extent();
    m_tree->tick(static_cast<float>(extent.width), static_cast<float>(extent.height));

    if (!m_tree->tree_dirty())
        return;

    m_tree->render(m_renderer);

    auto& visual = world.component<graphics::visual>(m_entity);
    visual.submeshes.clear();
    visual.materials.clear();

    std::size_t vertex_offset = 0;
    std::size_t index_offset = 0;

    for (auto& batch : m_renderer)
    {
        m_vertex_buffers[0]->upload(
            batch->vertex_position.data(),
            batch->vertex_position.size() * sizeof(math::float3),
            vertex_offset * sizeof(math::float3));
        m_vertex_buffers[1]->upload(
            batch->vertex_uv.data(),
            batch->vertex_uv.size() * sizeof(math::float2),
            vertex_offset * sizeof(math::float2));
        m_vertex_buffers[2]->upload(
            batch->vertex_color.data(),
            batch->vertex_color.size() * sizeof(std::uint32_t),
            vertex_offset * sizeof(std::uint32_t));
        m_index_buffer->upload(
            batch->indices.data(),
            batch->indices.size() * sizeof(std::uint32_t),
            index_offset * sizeof(std::uint32_t));

        graphics::submesh submesh = {
            .index_start = index_offset,
            .index_end = index_offset + batch->indices.size(),
            .vertex_base = vertex_offset};
        visual.submeshes.push_back(submesh);

        auto material_parameter = allocate_material_parameter();
        material_parameter->set(0, static_cast<std::uint32_t>(batch->type));
        if (batch->type != RENDER_TYPE_BLOCK)
            material_parameter->set(1, batch->texture);

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameters = {material_parameter, m_mvp_parameter.get()};
        material.scissor = graphics::scissor_extent{
            .min_x = static_cast<std::uint32_t>(batch->scissor.x),
            .min_y = static_cast<std::uint32_t>(batch->scissor.y),
            .max_x = static_cast<std::uint32_t>(batch->scissor.x + batch->scissor.width),
            .max_y = static_cast<std::uint32_t>(batch->scissor.y + batch->scissor.height)};

        visual.materials.push_back(material);

        vertex_offset += batch->vertex_position.size();
        index_offset += batch->indices.size();
    }

    m_material_parameter_counter = 0;
}

void ui::resize(std::uint32_t width, std::uint32_t height)
{
    float L = 0.0f;
    float R = static_cast<float>(width);
    float T = 0.0f;
    float B = static_cast<float>(height);
    m_mvp_parameter->set(
        0,
        math::float4x4{
            math::float4{2.0f / (R - L),    0.0f,              0.0f, 0.0f},
            math::float4{0.0f,              2.0f / (T - B),    0.0f, 0.0f},
            math::float4{0.0f,              0.0f,              0.5f, 0.0f},
            math::float4{(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f}
    });
}

graphics::pipeline_parameter* ui::allocate_material_parameter()
{
    if (m_material_parameter_counter >= m_material_parameter_pool.size())
        m_material_parameter_pool.push_back(
            system<graphics::graphics>().make_pipeline_parameter("ui_material"));

    auto result = m_material_parameter_pool[m_material_parameter_counter].get();
    result->reset();
    ++m_material_parameter_counter;

    return result;
}
} // namespace ash::ui
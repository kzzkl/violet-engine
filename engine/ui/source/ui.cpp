#include "ui/ui.hpp"
#include "core/relation.hpp"
#include "graphics/graphics.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"

namespace ash::ui
{
ui::ui() : system_base("ui"), m_material_parameter_counter(0)
{
}

bool ui::initialize(const dictionary& config)
{
    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    world.register_component<element>();

    m_font = std::make_unique<font_type>("engine/font/consola.ttf", 13);
    m_tree = std::make_unique<element_tree>();

    m_pipeline = std::make_unique<ui_pipeline>();
    m_mvp_parameter = graphics.make_pipeline_parameter("ui_mvp");

    m_vertex_buffers.push_back(graphics.make_vertex_buffer<math::float2>(
        nullptr,
        2048,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true));
    m_vertex_buffers.push_back(graphics.make_vertex_buffer<math::float2>(
        nullptr,
        2048,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true));
    m_vertex_buffers.push_back(graphics.make_vertex_buffer<std::uint32_t>(
        nullptr,
        2048,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true));
    m_index_buffer = graphics.make_index_buffer<std::uint32_t>(nullptr, 4096, true);

    m_entity = world.create("ui root");
    world.add<graphics::visual>(m_entity);

    auto& visual = world.component<graphics::visual>(m_entity);
    visual.groups = graphics::VISUAL_GROUP_UI;
    for (auto& vertex_buffer : m_vertex_buffers)
        visual.vertex_buffers.push_back(vertex_buffer.get());
    visual.index_buffer = m_index_buffer.get();

    event.subscribe<core::event_link>("ui", [&, this](ecs::entity entity, core::link& link) {
        if (world.has_component<element>(entity) && world.has_component<element>(link.parent))
        {
            auto& child = world.component<element>(entity);
            auto& parent = world.component<element>(link.parent);

            m_tree->link(child, parent);
        }
    });

    event.subscribe<window::event_window_resize>(
        "ui",
        [this](std::uint32_t width, std::uint32_t height) { resize(width, height); });

    auto window_extent = system<window::window>().extent();
    resize(window_extent.width, window_extent.height);

    return true;
}

void ui::begin_frame()
{
}

void ui::end_frame()
{
    auto& world = system<ecs::world>();

    if (!m_tree->tick())
        return;

    auto& visual = world.component<graphics::visual>(m_entity);
    visual.submeshes.clear();
    visual.materials.clear();

    std::size_t vertex_offset = 0;
    std::size_t index_offset = 0;

    for (auto& [key, mesh] : *m_tree)
    {
        m_vertex_buffers[0]->upload(
            mesh.vertex_position.data(),
            mesh.vertex_position.size() * sizeof(math::float2),
            vertex_offset * sizeof(math::float2));
        m_vertex_buffers[1]->upload(
            mesh.vertex_uv.data(),
            mesh.vertex_uv.size() * sizeof(math::float2),
            vertex_offset * sizeof(math::float2));
        m_vertex_buffers[2]->upload(
            mesh.vertex_color.data(),
            mesh.vertex_color.size() * sizeof(std::uint32_t),
            vertex_offset * sizeof(std::uint32_t));
        m_index_buffer->upload(
            mesh.indices.data(),
            mesh.indices.size() * sizeof(std::uint32_t),
            index_offset * sizeof(std::uint32_t));

        graphics::submesh submesh = {
            .index_start = index_offset,
            .index_end = index_offset + mesh.indices.size(),
            .vertex_base = vertex_offset};
        visual.submeshes.push_back(submesh);

        auto material_parameter = allocate_material_parameter();
        material_parameter->set(0, static_cast<std::uint32_t>(key.type));
        if (key.type != ELEMENT_CONTROL_TYPE_BLOCK)
            material_parameter->set(1, mesh.texture);

        graphics::material material = {
            .pipeline = m_pipeline.get(),
            .parameters = {material_parameter, m_mvp_parameter.get()}
        };
        visual.materials.push_back(material);

        vertex_offset += mesh.vertex_position.size();
        index_offset += mesh.indices.size();
    }

    m_material_parameter_counter = 0;
}

void ui::resize(std::uint32_t width, std::uint32_t height)
{
    m_tree->resize(width, height);

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

    ++m_material_parameter_counter;
    return m_material_parameter_pool[m_material_parameter_counter - 1].get();
}
} // namespace ash::ui
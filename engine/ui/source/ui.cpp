#include "ui.hpp"
#include "graphics.hpp"
#include "relation.hpp"
#include "window.hpp"
#include "window_event.hpp"

namespace ash::ui
{
ui::ui() : system_base("ui"), m_parameter_counter(0)
{
}

bool ui::initialize(const dictionary& config)
{
    m_font = std::make_unique<font>("engine/font/Ruda-Bold.ttf", 36);

    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    m_pipeline = std::make_unique<ui_pipeline>();

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
    m_index_buffer = graphics.make_index_buffer<std::uint32_t>(nullptr, 4096, true);

    m_root = world.create("ui root");
    world.add<graphics::visual>(m_root);

    auto& visual = world.component<graphics::visual>(m_root);
    visual.groups = graphics::VISUAL_GROUP_UI;

    visual.submeshes.resize(1);
    for (auto& vertex_buffer : m_vertex_buffers)
        visual.vertex_buffers.push_back(vertex_buffer.get());
    visual.index_buffer = m_index_buffer.get();

    visual.materials.resize(1);
    visual.materials[0].pipeline = m_pipeline.get();
    visual.materials[0].parameters.push_back(allocate_parameter());

    return true;
}

void ui::window(std::string_view label)
{
}

bool ui::window_ex(std::string_view label)
{
    return true;
}

void ui::window_root(std::string_view label)
{
}

void ui::window_pop()
{
}

void ui::text(std::string_view text, const element_rect& rect)
{
    m_tree.text(text, *m_font, rect);
}

bool ui::tree(std::string_view label, bool leaf)
{
    return true;
}

std::tuple<bool, bool> ui::tree_ex(std::string_view label, bool leaf)
{
    return {false, false};
}

void ui::tree_pop()
{
}

bool ui::collapsing(std::string_view label)
{
    return true;
}

void ui::texture(graphics::resource* texture, const element_rect& rect)
{
    m_tree.texture(texture, rect);
}

void ui::begin_frame()
{
}

void ui::end_frame()
{
    auto& world = system<ecs::world>();

    m_vertex_buffers[0]->upload(
        m_tree.vertex_position().data(),
        m_tree.vertex_position().size() * sizeof(math::float2));
    m_vertex_buffers[1]->upload(
        m_tree.vertex_uv().data(),
        m_tree.vertex_uv().size() * sizeof(math::float2));
    m_index_buffer->upload(
        m_tree.indices().data(),
        m_tree.indices().size() * sizeof(std::uint32_t));

    auto& elements = m_tree.elements();

    auto& visual = world.component<graphics::visual>(m_root);
    visual.submeshes[0].index_start = elements[1].index_start;
    visual.submeshes[0].index_end = elements[1].index_end;
    visual.submeshes[0].vertex_base = elements[1].vertex_base;

    float L = 0.0f;
    float R = 1300.0f;
    float T = 0.0f;
    float B = 800.0f;
    visual.materials[0].parameters[0]->set(
        0,
        math::float4x4{
            math::float4{2.0f / (R - L),    0.0f,              0.0f, 0.0f},
            math::float4{0.0f,              2.0f / (T - B),    0.0f, 0.0f},
            math::float4{0.0f,              0.0f,              0.5f, 0.0f},
            math::float4{(R + L) / (L - R), (T + B) / (B - T), 0.5f, 1.0f}
    });
    visual.materials[0].parameters[0]->set(1, m_font->texture());

    m_tree.clear();
}

graphics::pipeline_parameter* ui::allocate_parameter()
{
    if (m_parameter_counter >= m_parameter_pool.size())
        m_parameter_pool.push_back(
            system<graphics::graphics>().make_pipeline_parameter("ui_material"));

    ++m_parameter_counter;
    return m_parameter_pool[m_parameter_counter - 1].get();
}
} // namespace ash::ui
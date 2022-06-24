#include "ui/ui.hpp"
#include "ecs/world.hpp"
#include "graphics/graphics_task.hpp"
#include "graphics/rhi.hpp"
#include "task/task_manager.hpp"
#include "ui/controls.hpp"
#include "ui/element_tree.hpp"
#include "ui/theme.hpp"
#include "ui/ui_pipeline.hpp"
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
    auto& world = system<ecs::world>();
    auto& event = system<core::event>();

    load_font("remixicon", "engine/font/remixicon.ttf", 24);
    load_font("NotoSans-Regular", "engine/font/NotoSans-Regular.ttf", 13);

    // Register ui pipeline parameter layout.
    graphics::rhi::register_pipeline_parameter_layout("ui_mvp", mvp_pipeline_parameter::layout());
    graphics::rhi::register_pipeline_parameter_layout(
        "ui_offset",
        offset_pipeline_parameter::layout());
    graphics::rhi::register_pipeline_parameter_layout(
        "ui_material",
        material_pipeline_parameter::layout());

    m_pipeline = std::make_unique<ui_pipeline>();
    m_mvp_parameter = std::make_unique<mvp_pipeline_parameter>();
    m_offset_parameter = std::make_unique<offset_pipeline_parameter>();

    m_tree = std::make_unique<element_tree>();

    // Position.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<math::float2>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // UV.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<math::float2>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // Color.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<std::uint32_t>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // Offset index.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<std::uint32_t>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    m_index_buffer =
        graphics::rhi::make_index_buffer<std::uint32_t>(nullptr, MAX_UI_INDEX_COUNT, true, true);

    m_entity = world.create("ui root");
    world.add<graphics::visual>(m_entity);

    auto& visual = world.component<graphics::visual>(m_entity);
    visual.groups = graphics::VISUAL_GROUP_UI;
    for (auto& vertex_buffer : m_vertex_buffers)
        visual.vertex_buffers.push_back(vertex_buffer.get());
    visual.index_buffer = m_index_buffer.get();

    event.subscribe<window::event_window_resize>(
        "ui",
        [this](std::uint32_t width, std::uint32_t height) { resize(width, height); });
    event.subscribe<window::event_keyboard_char>("ui", [this](char c) { m_tree->input(c); });

    auto window_extent = system<window::window>().extent();
    resize(window_extent.width, window_extent.height);

    auto& task = system<task::task_manager>();
    auto ui_tick_task = task.schedule(TASK_UI_TICK, [this]() { tick(); });
    ui_tick_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_END));

    auto render_task = task.find(graphics::TASK_GRAPHICS_RENDER);
    render_task->add_dependency(*ui_tick_task);

    initialize_default_theme();

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

    m_renderer.draw(m_tree.get());
    m_offset_parameter->offset(m_renderer.offset());

    auto& visual = world.component<graphics::visual>(m_entity);
    visual.submeshes.clear();
    visual.materials.clear();

    std::size_t vertex_offset = 0;
    std::size_t index_offset = 0;

    for (auto& batch : m_renderer)
    {
        m_vertex_buffers[0]->upload(
            batch->vertex_position.data(),
            batch->vertex_position.size() * sizeof(math::float2),
            vertex_offset * sizeof(math::float2));
        m_vertex_buffers[1]->upload(
            batch->vertex_uv.data(),
            batch->vertex_uv.size() * sizeof(math::float2),
            vertex_offset * sizeof(math::float2));
        m_vertex_buffers[2]->upload(
            batch->vertex_color.data(),
            batch->vertex_color.size() * sizeof(std::uint32_t),
            vertex_offset * sizeof(std::uint32_t));
        m_vertex_buffers[3]->upload(
            batch->vertex_offset_index.data(),
            batch->vertex_offset_index.size() * sizeof(std::uint32_t),
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
        material_parameter->mesh_type(batch->type);
        if (batch->type != ELEMENT_MESH_TYPE_BLOCK)
            material_parameter->texture(batch->texture);

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameters = {
            material_parameter->interface(),
            m_offset_parameter->interface(),
            m_mvp_parameter->interface()};
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

void ui::load_font(std::string_view name, std::string_view ttf_file, std::size_t size)
{
    m_fonts[name.data()] = std::make_unique<font_type>(ttf_file, size);
}

const ui::font_type* ui::font(std::string_view name)
{
    auto iter = m_fonts.find(name.data());
    if (iter != m_fonts.end())
        return iter->second.get();
    else
        throw std::out_of_range("Font not found.");
}

const ui::font_type* ui::default_text_font()
{
    return font("NotoSans-Regular");
}

const ui::font_type* ui::default_icon_font()
{
    return font("remixicon");
}

element* ui::root() const noexcept
{
    return m_tree.get();
}

void ui::initialize_default_theme()
{
    auto text_font = default_text_font();
    auto icon_font = default_icon_font();

    // Dark.
    {
        std::string theme_name = "dark";

        button_theme button_dark = {
            .text_font = text_font,
            .text_color = COLOR_WHITE,
            .default_color = 0xFF3B291E,
            .highlight_color = 0xFFB2CC38};
        register_theme(theme_name, button_dark);

        icon_button_theme icon_button_dark = {
            .icon_font = icon_font,
            .icon_scale = 1.0f,
            .default_color = 0xFF3B291E,
            .highlight_color = 0xFFB2CC38};
        register_theme(theme_name, icon_button_dark);

        collapse_theme collapse_dark = {
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 0.7f,
            .title_font = text_font,
            .title_color = COLOR_WHITE,
            .bar_color = 0xFFB2CC38,
            .container_color = 0xFF5F4B3D};
        register_theme(theme_name, collapse_dark);

        dock_area_theme dock_area_dark = {.hover_color = 0xFF3B291E};
        register_theme(theme_name, dock_area_dark);

        dock_window_theme dock_window_dark = {
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 0.7f,
            .title_font = text_font,
            .title_color = COLOR_WHITE,
            .scroll_speed = 30.0f,
            .bar_width = 8.0f,
            .bar_color = 0xFF3B291E,
            .slider_color = 0xFFB2CC38,
            .container_color = 0xFF5F4B3D};
        register_theme(theme_name, dock_window_dark);

        font_icon_theme font_icon_dark = {
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 1.0f};
        register_theme(theme_name, font_icon_dark);

        input_theme input_dark = {
            .text_font = text_font,
            .text_color = COLOR_WHITE,
            .background_color = 0xFF3B291E,
            .underline_color = 0xFFB2CC38,
            .select_color = 0xFFB2CC38};
        register_theme(theme_name, input_dark);

        label_theme label_dark = {.text_font = text_font, .text_color = COLOR_WHITE};
        register_theme(theme_name, label_dark);

        scroll_view_theme scroll_view_dark = {
            .scroll_speed = 30.0f,
            .bar_width = 8.0f,
            .bar_color = 0xFF3B291E,
            .slider_color = 0xFFB2CC38,
            .background_color = 0xFF5F4B3D};
        register_theme(theme_name, scroll_view_dark);

        tree_node_theme tree_node_dark = {
            .text_font = text_font,
            .text_color = COLOR_WHITE,
            .icon_font = icon_font,
            .icon_color = COLOR_WHITE,
            .icon_scale = 0.7f,
            .padding_top = 5.0f,
            .padding_bottom = 5.0f,
            .padding_increment = 20.0f,
            .default_color = 0xFF3B291E,
            .highlight_color = 0xFFB2CC38};
        register_theme(theme_name, tree_node_dark);
    }
}

void ui::resize(std::uint32_t width, std::uint32_t height)
{
    float L = 0.0f;
    float R = static_cast<float>(width);
    float T = 0.0f;
    float B = static_cast<float>(height);
    math::float4x4 orthographic = math::matrix_plain::orthographic(
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f,
        0.0f,
        1.0f);
    m_mvp_parameter->mvp_matrix(orthographic);
    m_tree->width(width);
    m_tree->height(height);
}

material_pipeline_parameter* ui::allocate_material_parameter()
{
    if (m_material_parameter_counter >= m_material_parameter_pool.size())
        m_material_parameter_pool.push_back(std::make_unique<material_pipeline_parameter>());

    auto result = m_material_parameter_pool[m_material_parameter_counter].get();
    ++m_material_parameter_counter;

    return result;
}
} // namespace ash::ui
#include "editor/component_view.hpp"
#include "core/context.hpp"
#include "ecs/world.hpp"
#include "scene/transform.hpp"
#include "ui/controls/input.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/ui.hpp"

namespace ash::editor
{
component_panel::component_panel(std::string_view component_name, const ui::collapse_theme& theme)
    : ui::collapse(component_name, theme)
{
    container()->padding(10.0f, ui::LAYOUT_EDGE_ALL);
}

class information_panel : public component_panel
{
public:
    information_panel(const ui::collapse_theme& theme);

    virtual void tick(bool entity_change, ecs::entity entity) override;

private:
    std::unique_ptr<ui::label> m_name;
};

information_panel::information_panel(const ui::collapse_theme& theme)
    : component_panel("Information", theme)
{
    auto& ui = system<ui::ui>();
    m_name = std::make_unique<ui::label>("none", ui.theme<ui::label_theme>("dark"));
    m_name->width(100.0f);
    m_name->height(30.0f);
    add_item(m_name.get());
}

void information_panel::tick(bool entity_change, ecs::entity entity)
{
    if (entity_change)
    {
        auto& world = system<ecs::world>();
        auto& info = world.component<ecs::information>(entity);
        m_name->text(info.name);
    }
}

class transform_panel : public component_panel
{
public:
    transform_panel(const ui::collapse_theme& theme);

    virtual void tick(bool entity_change, ecs::entity entity) override;

private:
    void initialize_position();
    void initialize_rotation();
    void initialize_scaling();

    std::unique_ptr<ui::element> m_position;
    std::array<std::unique_ptr<ui::label>, 4> m_position_label;
    std::array<std::unique_ptr<ui::input_float>, 3> m_position_input;

    std::unique_ptr<ui::element> m_rotation;
    std::array<std::unique_ptr<ui::label>, 4> m_rotation_label;
    std::array<std::unique_ptr<ui::input_float>, 3> m_rotation_input;

    std::unique_ptr<ui::element> m_scaling;
    std::array<std::unique_ptr<ui::label>, 4> m_scaling_label;
    std::array<std::unique_ptr<ui::input_float>, 3> m_scaling_input;

    ecs::entity m_entity;
};

transform_panel::transform_panel(const ui::collapse_theme& theme)
    : component_panel("Transform", theme)
{
    initialize_position();
    initialize_rotation();
    initialize_scaling();
}

void transform_panel::tick(bool entity_change, ecs::entity entity)
{
    if (entity_change)
        m_entity = entity;

    auto& world = system<ecs::world>();

    auto& transform = world.component<scene::transform>(m_entity);
    auto euler = math::euler_plain::rotation_quaternion(transform.rotation);
    for (std::size_t i = 0; i < 3; ++i)
    {
        // Update position.
        if (m_position_input[i]->value() != transform.position[i])
            m_position_input[i]->value(transform.position[i]);

        // Update rotation.
        if (m_rotation_input[i]->value() != euler[i])
            m_rotation_input[i]->value(euler[i]);

        // Update scaling.
        if (m_scaling_input[i]->value() != transform.scaling[i])
            m_scaling_input[i]->value(transform.scaling[i]);
    }
}

void transform_panel::initialize_position()
{
    auto& ui = system<ui::ui>();
    auto& world = system<ecs::world>();

    auto label_theme = ui.theme<ui::label_theme>("dark");
    auto input_theme = ui.theme<ui::input_theme>("dark");

    m_position = std::make_unique<ui::element>();
    m_position->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW_REVERSE);
    m_position->margin(5.0f, ui::LAYOUT_EDGE_BOTTOM);
    add_item(m_position.get());

    m_position_label[0] = std::make_unique<ui::label>("X", label_theme);
    m_position_label[1] = std::make_unique<ui::label>("Y", label_theme);
    m_position_label[2] = std::make_unique<ui::label>("Z", label_theme);
    m_position_label[3] = std::make_unique<ui::label>("Position", label_theme);
    m_position_label[3]->flex_grow(1.0f);

    for (int i = 2; i >= 0; --i)
    {
        m_position_input[i] = std::make_unique<ui::input_float>(input_theme);
        m_position_input[i]->width(80.0f);
        m_position_input[i]->on_value_change = [&, i, this](float value) {
            auto& transform = world.component<scene::transform>(m_entity);
            transform.position[i] = value;
            transform.dirty = true;
        };
        m_position->add(m_position_input[i].get());

        m_position_label[i]->width(20.0f);
        m_position_label[i]->margin(10.0f, ui::LAYOUT_EDGE_LEFT);
        m_position->add(m_position_label[i].get());
    }
    m_position->add(m_position_label[3].get());
}

void transform_panel::initialize_rotation()
{
    auto& ui = system<ui::ui>();
    auto& world = system<ecs::world>();

    auto label_theme = ui.theme<ui::label_theme>("dark");
    auto input_theme = ui.theme<ui::input_theme>("dark");

    m_rotation = std::make_unique<ui::element>();
    m_rotation->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW_REVERSE);
    m_rotation->margin(5.0f, ui::LAYOUT_EDGE_BOTTOM);
    add_item(m_rotation.get());

    m_rotation_label[0] = std::make_unique<ui::label>("X", label_theme);
    m_rotation_label[1] = std::make_unique<ui::label>("Y", label_theme);
    m_rotation_label[2] = std::make_unique<ui::label>("Z", label_theme);
    m_rotation_label[3] = std::make_unique<ui::label>("Rotation", label_theme);
    m_rotation_label[3]->flex_grow(1.0f);

    for (int i = 2; i >= 0; --i)
    {
        m_rotation_input[i] = std::make_unique<ui::input_float>(input_theme);
        m_rotation_input[i]->width(80.0f);
        m_rotation_input[i]->on_value_change = [&, i, this](float value) {
            auto& transform = world.component<scene::transform>(m_entity);

            auto euler = math::euler_plain::rotation_quaternion(transform.rotation);
            euler[i] = value;
            transform.rotation = math::quaternion_plain::rotation_euler(euler);
            transform.dirty = true;
        };
        m_rotation->add(m_rotation_input[i].get());

        m_rotation_label[i]->width(20.0f);
        m_rotation_label[i]->margin(10.0f, ui::LAYOUT_EDGE_LEFT);
        m_rotation->add(m_rotation_label[i].get());
    }
    m_rotation->add(m_rotation_label[3].get());
}

void transform_panel::initialize_scaling()
{
    auto& ui = system<ui::ui>();
    auto& world = system<ecs::world>();

    auto label_theme = ui.theme<ui::label_theme>("dark");
    auto input_theme = ui.theme<ui::input_theme>("dark");

    m_scaling = std::make_unique<ui::element>();
    m_scaling->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW_REVERSE);
    m_scaling->margin(5.0f, ui::LAYOUT_EDGE_BOTTOM);
    add_item(m_scaling.get());

    m_scaling_label[0] = std::make_unique<ui::label>("X", label_theme);
    m_scaling_label[1] = std::make_unique<ui::label>("Y", label_theme);
    m_scaling_label[2] = std::make_unique<ui::label>("Z", label_theme);
    m_scaling_label[3] = std::make_unique<ui::label>("Scaling", label_theme);
    m_scaling_label[3]->flex_grow(1.0f);

    for (int i = 2; i >= 0; --i)
    {
        m_scaling_input[i] = std::make_unique<ui::input_float>(input_theme);
        m_scaling_input[i]->width(80.0f);
        m_scaling_input[i]->on_value_change = [&, i, this](float value) {
            auto& transform = world.component<scene::transform>(m_entity);
            transform.scaling[i] = value;
            transform.dirty = true;
        };
        m_scaling->add(m_scaling_input[i].get());

        m_scaling_label[i]->width(20.0f);
        m_scaling_label[i]->margin(10.0f, ui::LAYOUT_EDGE_LEFT);
        m_scaling->add(m_scaling_label[i].get());
    }
    m_scaling->add(m_scaling_label[3].get());
}

component_view::component_view(ui::dock_area* area, const ui::dock_window_theme& theme)
    : ui::dock_window("Component", 0xF161, area, theme),
      m_current_entity(ecs::INVALID_ENTITY)
{
    register_component<ecs::information, information_panel>();
    register_component<scene::transform, transform_panel>();
}

void component_view::tick(ecs::entity entity)
{
    if (m_current_entity == entity)
    {
        for (auto panel : m_current_panels)
        {
            if (panel->is_open())
                panel->tick(false, entity);
        }
    }
    else
    {
        auto& world = system<ecs::world>();
        if (m_component_mask != world.mask(entity))
        {
            sync_component_panel(entity);
            m_component_mask = world.mask(entity);
        }

        for (auto panel : m_current_panels)
            panel->tick(true, entity);

        m_current_entity = entity;
    }
}

void component_view::sync_component_panel(ecs::entity entity)
{
    auto& world = system<ecs::world>();

    for (auto component_panel : m_current_panels)
        component_panel->hide();
    m_current_panels.clear();

    if (entity == ecs::INVALID_ENTITY)
        return;

    for (auto component : world.components(entity))
    {
        if (component < m_component_panels.size() && m_component_panels[component] != nullptr)
        {
            m_component_panels[component]->show();
            m_current_panels.push_back(m_component_panels[component].get());
        }
    }
}
} // namespace ash::editor
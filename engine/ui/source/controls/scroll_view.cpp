#include "ui/controls/scroll_view.hpp"
#include "log.hpp"

namespace ash::ui
{
scroll_bar::scroll_bar(bool vertical, std::uint32_t slider_color, std::uint32_t bar_color)
    : panel(bar_color),
      m_vertical(vertical),
      m_position(0.0f),
      m_slider(std::make_unique<panel>(slider_color))
{
    if (vertical)
    {
        flex_direction(LAYOUT_FLEX_DIRECTION_COLUMN);
        on_mouse_drag = [this](int x, int y) -> bool {
            auto& bar_extent = extent();
            auto& slider_extent = m_slider->extent();
            float old_position = m_position;
            m_position =
                std::max(static_cast<float>(y) - slider_extent.height * 0.5f, bar_extent.y);
            m_position =
                std::min(bar_extent.y + bar_extent.height - slider_extent.height, m_position);
            if (m_position != old_position)
            {
                m_slider->position(m_position - bar_extent.y, LAYOUT_EDGE_TOP);
                if (on_slide)
                    on_slide(value());
            }

            return false;
        };
    }
    else
    {
        flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
        on_mouse_drag = [this](int x, int y) -> bool {
            auto& bar_extent = extent();
            auto& slider_extent = m_slider->extent();
            float old_position = m_position;
            m_position = std::max(static_cast<float>(x) - slider_extent.width * 0.5f, bar_extent.x);
            m_position =
                std::min(bar_extent.x + bar_extent.width - slider_extent.width, m_position);
            if (m_position != old_position)
            {
                m_slider->position(m_position - bar_extent.x, LAYOUT_EDGE_LEFT);
                if (on_slide)
                    on_slide(value());
            }

            return false;
        };
    }

    m_slider->resize(100.0f, 100.0f, false, false, true, true);
    m_slider->link(this);
    m_slider->name = "m_slider";
}

void scroll_bar::value(float v) noexcept
{
    float old_value = value();
    if (v == old_value)
        return;

    auto& bar_extent = extent();
    auto& slider_extent = m_slider->extent();

    if (m_vertical)
    {
        m_position = v * (bar_extent.height - slider_extent.height) + bar_extent.y;
        m_slider->position(m_position - bar_extent.y, LAYOUT_EDGE_TOP);
    }
    else
    {
        m_position = v * (bar_extent.width - slider_extent.width) + bar_extent.x;
        m_slider->position(m_position - bar_extent.x, LAYOUT_EDGE_LEFT);
    }
}

float scroll_bar::value() const noexcept
{
    auto& bar_extent = extent();
    auto& slider_extent = m_slider->extent();
    if (m_vertical)
        return (m_position - bar_extent.y) / (bar_extent.height - slider_extent.height);
    else
        return (m_position - bar_extent.x) / (bar_extent.width - slider_extent.width);
}

scroll_container::scroll_container(scroll_bar* vertical_bar, scroll_bar* horizontal_bar)
    : m_vertical_bar(vertical_bar),
      m_horizontal_bar(horizontal_bar)
{
}

void scroll_container::on_extent_change()
{
    auto& view_extent = parent()->extent();
    auto& container_extent = extent();

    if (container_extent.height <= view_extent.height)
    {
        m_vertical_bar->hide();
        position(0.0f, LAYOUT_EDGE_TOP);
        position(0.0f, LAYOUT_EDGE_LEFT);
    }
    else
    {
        m_vertical_bar->show();
        if (m_height != container_extent.height)
        {
            float slider_height = math::clamp(
                view_extent.height * view_extent.height / container_extent.height,
                30.0f,
                view_extent.height);
            m_vertical_bar->slider()->resize(100.0f, slider_height, false, false, true, false);

            m_height = container_extent.height;
        }
    }

    if (container_extent.width <= view_extent.width)
    {
        m_horizontal_bar->hide();
    }
    else
    {
        m_horizontal_bar->show();
        if (m_height != container_extent.height)
        {
            float slider_width = math::clamp(
                view_extent.width * view_extent.width / container_extent.width,
                30.0f,
                view_extent.width);
            m_horizontal_bar->slider()->resize(slider_width, 100.0f, false, false, false, true);

            m_height = container_extent.height;
        }
    }
}

scroll_view::scroll_view() : panel(COLOR_BROWN)
{
    name = "scroll_view";
    m_vertical_bar = std::make_unique<scroll_bar>(true);
    m_vertical_bar->resize(8.0f, 100.0f, false, false, false, true);
    m_vertical_bar->position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    m_vertical_bar->position(0.0f, LAYOUT_EDGE_RIGHT);
    m_vertical_bar->on_slide = [this](float value) { sync_container_vertical_position(value); };
    m_vertical_bar->layer(10);
    m_vertical_bar->link(this);
    m_vertical_bar->name = "m_vertical_bar";

    m_horizontal_bar = std::make_unique<scroll_bar>(false);
    m_horizontal_bar->resize(100.0f, 8.0f, false, false, true, false);
    m_horizontal_bar->position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    m_horizontal_bar->position(0.0f, LAYOUT_EDGE_BOTTOM);
    m_horizontal_bar->on_slide = [this](float value) { sync_container_horizontal_position(value); };
    m_horizontal_bar->layer(10);
    m_horizontal_bar->link(this);

    m_container_view = std::make_unique<view>();
    m_container_view->flex_grow(1.0f);
    m_container_view->link(this);
    m_container_view->name = "m_container_view";
    m_container = std::make_unique<scroll_container>(m_vertical_bar.get(), m_horizontal_bar.get());
    m_container->position_type(LAYOUT_POSITION_TYPE_ABSOLUTE);
    m_container->link(m_container_view.get());
    m_container->name = "m_container";

    flex_direction(LAYOUT_FLEX_DIRECTION_ROW);
    on_mouse_wheel = [this](int whell) -> bool {
        if (m_vertical_bar->display())
        {
            auto& container_extent = m_container->extent();

            float new_value = m_vertical_bar->value() - 20.0f / container_extent.height * whell;
            new_value = math::clamp(new_value, 0.0f, 1.0f);
            m_vertical_bar->value(new_value);
            sync_container_vertical_position(new_value);
        }
        return false;
    };
}

void scroll_view::sync_container_vertical_position(float bar_value)
{
    auto& container_extent = m_container->extent();
    auto& view_extent = extent();
    m_container->position(
        bar_value * (view_extent.height - container_extent.height),
        LAYOUT_EDGE_TOP);
}

void scroll_view::sync_container_horizontal_position(float bar_value)
{
    auto& container_extent = m_container->extent();
    auto& view_extent = extent();
    m_container->position(
        bar_value * (view_extent.width - container_extent.width),
        LAYOUT_EDGE_LEFT);
}
} // namespace ash::ui
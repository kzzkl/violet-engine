#include "gallery.hpp"
#include "style.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
gallery::gallery() : m_current_page(nullptr)
{
}

void gallery::initialize()
{
    auto& ui = system<ui::ui>();

    ui.load_font(style::title_1.font, style::title_1.font_path, style::title_1.font_size);
    ui.load_font(style::title_2.font, style::title_2.font_path, style::title_2.font_size);
    ui.load_font(style::content.font, style::content.font_path, style::content.font_size);

    m_left = std::make_unique<ui::panel>(style::background_color);
    m_left->link(ui.root());

    m_navigation_node_style = ui::tree_node::default_style;
    m_navigation_node_style.padding_top = 8.0f;
    m_navigation_node_style.padding_bottom = 8.0f;
    m_navigation_node_style.padding_increment = 10.0f;
    m_navigation_node_style.icon_scale = 0.7f;

    m_navigation_tree = std::make_unique<ui::tree>();
    m_navigation_tree->width_min(300.0f);
    m_navigation_tree->on_select = [this](ui::tree_node* node) {
        auto& page = m_pages[node->name()];
        if (page != nullptr && m_current_page != page.get())
        {
            if (m_current_page != nullptr)
                m_main_view->remove(m_current_page);

            m_main_view->add(page.get());
            m_current_page = page.get();
        }
    };
    m_navigation_tree->link(m_left.get());

    m_main_view = std::make_unique<ui::scroll_view>();
    m_main_view->flex_grow(1.0f);
    m_main_view->link(ui.root());

    initialize_basic();
    initialize_views();
}

void gallery::initialize_basic()
{
    auto& text_font = system<ui::ui>().font("content");
    auto& icon_font = system<ui::ui>().font("remixicon");

    m_basic_node = std::make_unique<ui::tree_node>(
        "Basic",
        text_font,
        0xEB82,
        icon_font,
        m_navigation_node_style);
    m_navigation_tree->add(m_basic_node.get());

    // Button.
    m_button_page_node = std::make_unique<ui::tree_node>(
        "Button",
        text_font,
        0xEB7E,
        icon_font,
        m_navigation_node_style);
    m_basic_node->add(m_button_page_node.get());

    m_pages["Button"] = std::make_unique<button_page>();
}

void gallery::initialize_views()
{
    auto& text_font = system<ui::ui>().font("content");
    auto& icon_font = system<ui::ui>().font("remixicon");

    m_views_node = std::make_unique<ui::tree_node>(
        "Views",
        text_font,
        0xEE84,
        icon_font,
        m_navigation_node_style);
    m_navigation_tree->add(m_views_node.get());

    // Tree.
    m_tree_page_node = std::make_unique<ui::tree_node>(
        "Tree",
        text_font,
        0xEEBA,
        icon_font,
        m_navigation_node_style);
    m_views_node->add(m_tree_page_node.get());

    m_pages["Tree"] = std::make_unique<tree_page>();
}
} // namespace ash::sample
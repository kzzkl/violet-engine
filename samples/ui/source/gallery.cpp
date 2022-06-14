#include "gallery.hpp"
#include "button_page.hpp"
#include "docking_page.hpp"
#include "image_page.hpp"
#include "scroll_page.hpp"
#include "style.hpp"
#include "tree_page.hpp"
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

    m_left = std::make_unique<ui::panel>(style::background_color);
    m_left->link(ui.root());

    m_navigation_node_style = {};
    m_navigation_node_style.padding_top = 8.0f;
    m_navigation_node_style.padding_bottom = 8.0f;
    m_navigation_node_style.padding_increment = 10.0f;
    m_navigation_node_style.icon_scale = 0.7f;
    m_navigation_node_style.icon_font = &system<ui::ui>().font(ui::DEFAULT_ICON_FONT);
    m_navigation_node_style.text_font = &system<ui::ui>().font(ui::DEFAULT_TEXT_FONT);

    m_navigation_tree = std::make_unique<ui::tree>();
    m_navigation_tree->width_min(300.0f);
    m_navigation_tree->on_select = [this](ui::tree_node* node) {
        auto& page = m_pages[node->text()];
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
    initialize_docking();
}

void gallery::initialize_basic()
{
    m_nodes["Basic"] = std::make_unique<ui::tree_node>("Basic", 0xEB82, m_navigation_node_style);
    m_navigation_tree->add(m_nodes["Basic"].get());

    // Button.
    m_pages["Button"] = std::make_unique<button_page>();
    m_nodes["Button"] = std::make_unique<ui::tree_node>("Button", 0xEB7E, m_navigation_node_style);
    m_nodes["Basic"]->add(m_nodes["Button"].get());

    // Image.
    m_pages["Image"] = std::make_unique<image_page>();
    m_nodes["Image"] = std::make_unique<ui::tree_node>("Image", 0xEE4A, m_navigation_node_style);
    m_nodes["Basic"]->add(m_nodes["Image"].get());
}

void gallery::initialize_views()
{
    m_nodes["Views"] = std::make_unique<ui::tree_node>("Views", 0xEE8F, m_navigation_node_style);
    m_navigation_tree->add(m_nodes["Views"].get());

    // Tree.
    m_pages["Tree"] = std::make_unique<tree_page>();
    m_nodes["Tree"] = std::make_unique<ui::tree_node>("Tree", 0xEEBA, m_navigation_node_style);
    m_nodes["Views"]->add(m_nodes["Tree"].get());

    // Scroll.
    m_pages["Scroll"] = std::make_unique<scroll_page>();
    m_nodes["Scroll"] = std::make_unique<ui::tree_node>("Scroll", 0xEEBA, m_navigation_node_style);
    m_nodes["Views"]->add(m_nodes["Scroll"].get());
}

void gallery::initialize_docking()
{
    auto& text_font = system<ui::ui>().font(ui::DEFAULT_TEXT_FONT);
    auto& icon_font = system<ui::ui>().font(ui::DEFAULT_ICON_FONT);

    m_nodes["Docking"] =
        std::make_unique<ui::tree_node>("Docking", 0xEE84, m_navigation_node_style);
    m_navigation_tree->add(m_nodes["Docking"].get());
    m_pages["Docking"] = std::make_unique<docking_page>();
}
} // namespace ash::sample
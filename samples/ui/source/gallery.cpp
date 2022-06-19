#include "gallery.hpp"
#include "button_page.hpp"
#include "docking_page.hpp"
#include "image_page.hpp"
#include "input_page.hpp"
#include "scroll_page.hpp"
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

    m_left = std::make_unique<ui::panel>(0xFF3B291E);
    ui.root()->add(m_left.get());

    m_navigation_tree = std::make_unique<ui::tree>();
    m_navigation_tree->width_min(300.0f);
    m_navigation_tree->on_select = [this](ui::tree_node* node) {
        auto& page = m_pages[node->text()];
        if (page != nullptr && m_current_page != page.get())
        {
            if (m_current_page != nullptr)
                m_main_view->remove_item(m_current_page);

            m_main_view->add_item(page.get());
            m_current_page = page.get();
        }
    };
    m_left->add(m_navigation_tree.get());

    m_main_view = std::make_unique<ui::scroll_view>(ui.theme<ui::scroll_view_theme>("dark"));
    m_main_view->flex_grow(1.0f);
    ui.root()->add(m_main_view.get());

    initialize_theme();
    initialize_navigation();
}

void gallery::initialize_theme()
{
    auto& ui = system<ui::ui>();

    ui.load_font("gallery title 1", "engine/font/NotoSans-SemiBold.ttf", 25);
    ui.load_font("gallery title 2", "engine/font/NotoSans-SemiBold.ttf", 20);

    ui::label_theme title_1 = {
        .text_font = ui.font("gallery title 1"),
        .text_color = ui::COLOR_WHITE};
    ui.register_theme("gallery title 1", title_1);

    ui::label_theme title_2 = {
        .text_font = ui.font("gallery title 2"),
        .text_color = ui::COLOR_WHITE};
    ui.register_theme("gallery title 2", title_2);

    ui::tree_node_theme navigation_node_theme = ui.theme<ui::tree_node_theme>("dark");
    navigation_node_theme.padding_top = 10.0f;
    navigation_node_theme.padding_bottom = 10.0f;
    navigation_node_theme.padding_increment = 10.0f;
    navigation_node_theme.icon_scale = 0.7f;
    ui.register_theme("gallery navigation", navigation_node_theme);
}

void gallery::initialize_navigation()
{
    auto& navigation_node_theme = system<ui::ui>().theme<ui::tree_node_theme>("gallery navigation");

    // Basic.
    m_nodes["Basic"] = std::make_unique<ui::tree_node>("Basic", 0xEB82, navigation_node_theme);
    m_navigation_tree->add_node(m_nodes["Basic"].get());

    // Button.
    m_pages["Button"] = std::make_unique<button_page>();
    m_nodes["Button"] = std::make_unique<ui::tree_node>("Button", 0xEB7E, navigation_node_theme);
    m_nodes["Basic"]->add_node(m_nodes["Button"].get());

    // Image.
    m_pages["Image"] = std::make_unique<image_page>();
    m_nodes["Image"] = std::make_unique<ui::tree_node>("Image", 0xEE4A, navigation_node_theme);
    m_nodes["Basic"]->add_node(m_nodes["Image"].get());

    // Form.
    m_nodes["Form"] = std::make_unique<ui::tree_node>("Form", 0xEB82, navigation_node_theme);
    m_navigation_tree->add_node(m_nodes["Form"].get());

    // Input.
    m_pages["Input"] = std::make_unique<input_page>();
    m_nodes["Input"] = std::make_unique<ui::tree_node>("Input", 0xEE4A, navigation_node_theme);
    m_nodes["Form"]->add_node(m_nodes["Input"].get());

    // Views.
    m_nodes["Views"] = std::make_unique<ui::tree_node>("Views", 0xEE8F, navigation_node_theme);
    m_navigation_tree->add_node(m_nodes["Views"].get());

    // Tree.
    m_pages["Tree"] = std::make_unique<tree_page>();
    m_nodes["Tree"] = std::make_unique<ui::tree_node>("Tree", 0xEEBA, navigation_node_theme);
    m_nodes["Views"]->add_node(m_nodes["Tree"].get());

    // Scroll.
    m_pages["Scroll"] = std::make_unique<scroll_page>();
    m_nodes["Scroll"] = std::make_unique<ui::tree_node>("Scroll", 0xEE98, navigation_node_theme);
    m_nodes["Views"]->add_node(m_nodes["Scroll"].get());

    // Docking.
    m_nodes["Docking"] = std::make_unique<ui::tree_node>("Docking", 0xEE84, navigation_node_theme);
    m_navigation_tree->add_node(m_nodes["Docking"].get());
    m_pages["Docking"] = std::make_unique<docking_page>();
}
} // namespace ash::sample
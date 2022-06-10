#include "docking_page.hpp"
#include "ui/ui.hpp"

namespace ash::sample
{
static constexpr std::uint32_t PANEL_COLOR[] = {
    ui::COLOR_ANTIQUE_WHITE,
    ui::COLOR_AQUA,
    ui::COLOR_AQUA_MARINE,
    ui::COLOR_AZURE,
    ui::COLOR_BEIGE,
    ui::COLOR_BISQUE,
    ui::COLOR_BLACK,
    ui::COLOR_BLANCHEDALMOND,
    ui::COLOR_BLUE,
    ui::COLOR_BROWN,
    ui::COLOR_DARK_SALMON,
    ui::COLOR_DARK_SEA_GREEN};
/*
void print_tree(ui::element* node, std::size_t block = 0)
{
    auto d = dynamic_cast<ui::dock_element*>(node);
    if (d != nullptr)
    {
        std::string b(block, '-');
        log::debug("|{} {}({}, {})", b, node->name, d->m_width, d->m_height);
    }

    for (ui::element* child : node->children())
    {
        print_tree(child, block + 5);
    }
}
*/

docking_page::docking_page() : m_width(50.0f)
{
    name = "docking_page";

    m_title = std::make_unique<text_title_1>("Docking");
    m_title->link(this);

    m_description = std::make_unique<text_content>("The dock_window allows the child elements to "
                                                   "dock on the four sides of the parent, similar "
                                                   "to the layout of the IDE.");
    m_description->link(this);

    for (std::size_t i = 0; i < 2; ++i)
    {
        auto display = std::make_unique<display_panel>();
        display->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);
        m_display.push_back(std::move(display));
    }
    initialize_sample_docking();
}
static std::size_t panel_counter = 0;
void docking_page::initialize_sample_docking()
{
    auto& ui = system<ui::ui>();

    m_display[0]->link(this);

    ui::dock_window_style root_style = {};
    root_style.icon_font = &ui.font(ui::DEFAULT_ICON_FONT);
    root_style.title_font = &ui.font(ui::DEFAULT_TEXT_FONT);
    root_style.bar_color = ui::COLOR_WHEAT;

    m_root = std::make_unique<ui::dock_window>("Root Window", 0xEA43, root_style);
    m_root->width(700.0f);
    m_root->height(400.0f);
    m_root->link(m_display[0].get());

    m_right = std::make_unique<ui::element>();
    m_right->margin(50.0f, ui::LAYOUT_EDGE_LEFT);
    m_right->link(m_display[0].get());

    ui::button_style button_style = {};
    button_style.text_font = &ui.font(ui::DEFAULT_TEXT_FONT);
    m_button = std::make_unique<ui::button>("Create Dock Window", button_style);
    m_button->height(45.0f);
    m_button->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        if (m_dock_panels.size() == sizeof(PANEL_COLOR) / sizeof(std::uint32_t))
            return false;

        ui::dock_window_style style = {};
        style.icon_font = &ui.font(ui::DEFAULT_ICON_FONT);
        style.title_font = &ui.font(ui::DEFAULT_TEXT_FONT);
        style.bar_color = PANEL_COLOR[m_dock_panels.size()];

        auto new_panel = std::make_unique<ui::dock_window>(
            "Window " + std::to_string(panel_counter++),
            0xEA43,
            style);
        new_panel->width(200.0f);
        new_panel->height(200.0f);
        new_panel->link(m_right.get());
        m_dock_panels.push_back(std::move(new_panel));

        return false;
    };
    m_button->link(this);

    /*
    m_button2 = std::make_unique<ui::button>("test", ui.font(ui::DEFAULT_TEXT_FONT));
    m_button2->height(45.0f);
    m_button2->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        print_tree(m_display[0].get());
        return false;
    };
    m_button2->link(this);
    */
}
} // namespace ash::sample
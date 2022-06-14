#include "docking_page.hpp"
#include "ui/controls/dock_window.hpp"
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

void print_tree(ui::element* node, std::size_t block = 0)
{
    auto d = dynamic_cast<ui::dock_element*>(node);
    std::string b(block, '-');
    if (d != nullptr)
    {
        auto [width, height] = d->dock_extent();
        log::debug("|{} {}({}, {})", b, node->name, width, height);
    }
    else
    {
        log::debug("|{} {}", b, node->name);
    }

    for (ui::element* child : node->children())
    {
        print_tree(child, block + 5);
    }
}

docking_page::docking_page()
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

    m_dock_area = std::make_unique<ui::dock_area>(700, 400);
    m_dock_area->link(m_display[0].get());
    m_dock_area->dock(make_dock_window());
    m_dock_area->name = "area";

    m_right = std::make_unique<ui::element>();
    m_right->margin(50.0f, ui::LAYOUT_EDGE_LEFT);
    m_right->link(m_display[0].get());

    ui::button_style button_style = {};
    button_style.text_font = &ui.font(ui::DEFAULT_TEXT_FONT);
    m_create_button = std::make_unique<ui::button>("Create Dock Window", button_style);
    m_create_button->height(45.0f);
    m_create_button->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        if (m_dock_windows.size() == sizeof(PANEL_COLOR) / sizeof(std::uint32_t))
            return false;

        auto new_panel = make_dock_window();
        new_panel->link(m_right.get());
        return false;
    };
    m_create_button->link(this);

    m_print_button = std::make_unique<ui::button>("Print Tree", button_style);
    m_print_button->height(45.0f);
    m_print_button->on_mouse_press = [this](window::mouse_key key, int x, int y) -> bool {
        print_tree(m_display[0].get());
        return false;
    };
    m_print_button->link(this);

    m_test_button = std::make_unique<ui::button>("Test", button_style);
    m_test_button->height(45.0f);
    m_test_button->on_mouse_press = [this](window::mouse_key key, int x, int y) -> bool {
        // m_root->move_down();
        return false;
    };
    m_test_button->link(this);
}

ui::dock_element* docking_page::make_dock_window()
{
    ++panel_counter;

    ui::dock_window_style style = {};
    style.bar_color = ui::COLOR_DARK_ORCHID;
    style.title_font = &system<ui::ui>().font(ui::DEFAULT_TEXT_FONT);
    style.container_color = PANEL_COLOR[panel_counter];

    std::string window_name = "Window " + std::to_string(panel_counter);
    auto result = std::make_unique<ui::dock_window>(window_name, m_dock_area.get(), style);
    result->width(200.0f);
    result->height(200.0f);
    result->name = window_name;

    ui::label_style label_style = {};
    label_style.text_font = &system<ui::ui>().font(ui::DEFAULT_TEXT_FONT);
    auto label = std::make_unique<ui::label>(window_name, label_style);
    label->margin(100.0f, ui::LAYOUT_EDGE_ALL);
    result->add(label.get());

    m_dock_windows.push_back(std::move(result));
    m_window_labels.push_back(std::move(label));

    return m_dock_windows.back().get();
}
} // namespace ash::sample
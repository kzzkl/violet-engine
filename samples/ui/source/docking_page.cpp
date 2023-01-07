#include "docking_page.hpp"
#include "ui/controls/dock_window.hpp"
#include "ui/ui.hpp"
#include <format>

namespace violet::sample
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

docking_page::docking_page() : page("Docking")
{
    add_description("The dock_window allows the child elements to "
                    "dock on the four sides of the parent, similar "
                    "to the layout of the IDE.");

    initialize_sample_docking();
}

static std::size_t panel_counter = 0;
void docking_page::initialize_sample_docking()
{
    auto& ui = system<ui::ui>();

    auto display_1 = add_display_panel();
    display_1->flex_direction(ui::LAYOUT_FLEX_DIRECTION_ROW);

    m_dock_area = std::make_unique<ui::dock_area>(700, 400, ui.theme<ui::dock_area_theme>("dark"));
    display_1->add(m_dock_area.get());
    m_dock_area->dock(make_dock_window());
    m_dock_area->name = "area";

    m_right = std::make_unique<ui::element>();
    m_right->margin(50.0f, ui::LAYOUT_EDGE_LEFT);
    display_1->add(m_right.get());

    m_create_button =
        std::make_unique<ui::button>("Create Dock Window", ui.theme<ui::button_theme>("dark"));
    m_create_button->height(45.0f);
    m_create_button->on_mouse_press = [&, this](window::mouse_key key, int x, int y) -> bool {
        if (m_dock_windows.size() == sizeof(PANEL_COLOR) / sizeof(std::uint32_t))
            return false;

        auto new_panel = make_dock_window();
        m_right->add(new_panel);
        return false;
    };
    add(m_create_button.get());

    m_print_button = std::make_unique<ui::button>("Print Tree", ui.theme<ui::button_theme>("dark"));
    m_print_button->height(45.0f);
    m_print_button->on_mouse_press = [=, this](window::mouse_key key, int x, int y) -> bool {
        print_tree(display_1);
        return false;
    };
    add(m_print_button.get());
}

ui::dock_element* docking_page::make_dock_window()
{
    auto& ui = system<ui::ui>();

    ++panel_counter;

    ui::dock_window_theme theme = ui.theme<ui::dock_window_theme>("dark");
    theme.container_color = PANEL_COLOR[panel_counter];

    std::string window_name = std::format("Window {}", panel_counter);
    auto result = std::make_unique<ui::dock_window>(window_name, 0xEA43, m_dock_area.get(), theme);
    result->width(200.0f);
    result->height(200.0f);
    result->name = window_name;

    auto label_theme = ui.theme<ui::label_theme>("dark");
    label_theme.text_color = ui::COLOR_BLACK;
    auto label = std::make_unique<ui::label>(window_name, label_theme);
    label->margin(100.0f, ui::LAYOUT_EDGE_ALL);
    result->add_item(label.get());

    m_dock_windows.push_back(std::move(result));
    m_window_labels.push_back(std::move(label));

    return m_dock_windows.back().get();
}
} // namespace violet::sample
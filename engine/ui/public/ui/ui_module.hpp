#pragma once

#include "core/engine_module.hpp"
#include "ui/font.hpp"
#include "ui/rendering/ui_pass.hpp"
#include "ui/widget.hpp"

namespace violet
{
class ui_renderer;
class ui_module : public engine_module
{
public:
    ui_module();

    virtual bool initialize(const dictionary& config) override;

    void tick();

    void load_font(std::string_view name, std::string_view ttf_file, std::size_t size);
    font* get_font(std::string_view name);

    font* get_default_text_font();
    font* get_default_icon_font();

private:
    void update_input();
    void update_layout(widget* root, std::uint32_t width, std::uint32_t height);
    void render(widget* root, ui_pass* pass);

    void bubble_event(widget* node, const widget_event& event)
    {
        while (node != nullptr && node->receive_event(event))
            node = node->get_parent();
    }

    std::vector<widget*> m_mouse_over_widgets;

    int m_prev_mouse_x{-1};
    int m_prev_mouse_y{-1};
    int m_mouse_press_x{-1};
    int m_mouse_press_y{-1};

    bool m_drag{false};
    mouse_key m_drag_key{MOUSE_KEY_LEFT};
    widget* m_drag_widget{nullptr};

    std::unordered_map<std::string, std::unique_ptr<font>> m_fonts;

    std::unique_ptr<ui_renderer> m_renderer;
};
}; // namespace violet
#pragma once

#include "ui/widget.hpp"

namespace violet
{
class node_editor : public widget
{
public:
    class pin : public widget
    {
    public:
        pin();

    private:
        virtual void on_paint(ui_painter* painter) override;
    };

    class node : public widget
    {
    public:
        static constexpr float title_size = 40.0f;

    public:
        node();

        void set_title(std::string_view title) { m_title = title; }
        void set_title_color(ui_color title_color) noexcept { m_title_color = title_color; }

        void add_pin();

    private:
        virtual void on_paint(ui_painter* painter) override;
        virtual bool on_drag_start(mouse_key key, int x, int y) override;
        virtual bool on_drag(mouse_key key, int x, int y) override;

        std::string m_title;
        ui_color m_title_color{ui_color::WHITE};

        int m_drag_offset_x{0};
        int m_drag_offset_y{0};
    };

public:
    node_editor();

    node* add_node();

private:
    virtual void on_paint(ui_painter* painter) override;
    virtual void on_paint_end(ui_painter* painter) override;
    virtual bool on_mouse_click(mouse_key key, int x, int y) override;
    virtual bool on_drag_start(mouse_key key, int x, int y) override;
    virtual bool on_drag(mouse_key key, int x, int y) override;

    int m_drag_start_x{0};
    int m_drag_start_y{0};
    int m_drag_offset_x{0};
    int m_drag_offset_y{0};
};
} // namespace violet
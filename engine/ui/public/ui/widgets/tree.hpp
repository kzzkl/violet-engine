#pragma once

#include "ui/color.hpp"
#include "ui/control.hpp"
#include <string>

namespace violet
{
class font;
class font_icon;
class label;
class panel;

struct tree_node_theme
{
    const font* text_font;
    std::uint32_t text_color;

    const font* icon_font;
    std::uint32_t icon_color;
    float icon_scale;

    float padding_top;
    float padding_bottom;
    float padding_increment;

    std::uint32_t default_color;
    std::uint32_t highlight_color;
};

class tree_node : public control
{
public:
    tree_node() = default;
    tree_node(std::string_view name, const tree_node_theme& theme);
    tree_node(std::string_view name, std::uint32_t icon_index, const tree_node_theme& theme);
    virtual ~tree_node() = default;

    virtual void add_node(tree_node* child);
    virtual void remove_node(tree_node* child);

    void text(std::string_view text);
    void icon(std::uint32_t index);
    void icon_scale(float scale);

    std::string text() const;

public:
    using on_expand_event = control_event<void()>;
    using on_collapse_event = control_event<void()>;

    on_expand_event::handle on_expand;
    on_collapse_event::handle on_collapse;

protected:
    virtual void on_select_node(tree_node* node);

private:
    friend class tree;

    float m_padding_increment;
    float m_child_margin_offset;

    bool m_selected;
    tree_node* m_parent_node;

    std::uint32_t m_default_color;
    std::uint32_t m_highlight_color;

    std::unique_ptr<panel> m_top;
    std::unique_ptr<font_icon> m_button;
    std::unique_ptr<font_icon> m_icon;
    std::unique_ptr<label> m_text;

    std::unique_ptr<control> m_container;
};

class tree : public tree_node
{
public:
    tree();

    virtual void add_node(tree_node* child) override;

public:
    std::function<void(tree_node*)> on_select;

protected:
    virtual void on_select_node(tree_node* node) override;

private:
    tree_node* m_selected_node;
};
} // namespace violet
#pragma once

#include "ui/controls/font_icon.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/element.hpp"
#include <string>

namespace ash::ui
{
struct tree_node_style
{
    float padding_top;
    float padding_bottom;
    float padding_increment;

    std::uint32_t text_color;
    std::uint32_t default_color;
    std::uint32_t highlight_color;

    float icon_scale;
};

class tree_node : public element
{
public:
    static constexpr tree_node_style default_style = {
        .padding_top = 5.0f,
        .padding_bottom = 5.0f,
        .padding_increment = 30.0f,
        .text_color = COLOR_BLACK,
        .default_color = 0xFFFAFAFA,
        .highlight_color = 0xFFD8D8D9,
        .icon_scale = 1.0f};

public:
    tree_node() = default;
    tree_node(
        std::string_view name,
        const font& font,
        const tree_node_style& style = default_style);
    tree_node(
        std::string_view name,
        const font& text_font,
        std::uint32_t icon_index,
        const font& icon_font,
        const tree_node_style& style = default_style);
    virtual ~tree_node() = default;

    virtual void add(tree_node* child);
    virtual void remove(tree_node* child);

    void text(std::string_view text, const font& font) { m_text->text(text, font); }
    void icon(std::uint32_t index, const font& font) { m_icon->icon(index, font); }
    void icon_scale(float scale) { m_icon->icon_scale(scale); }

    std::string text() const { return m_text->text(); }

    void* user_data;

    std::function<void()> on_expand;
    std::function<void()> on_collapse;

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

    std::unique_ptr<element> m_container;
};

class tree : public tree_node
{
public:
    tree();

    virtual void add(tree_node* child) override;

    std::function<void(tree_node*)> on_select;

protected:
    virtual void on_select_node(tree_node* node) override;

private:
    tree_node* m_selected_node;
};
} // namespace ash::ui
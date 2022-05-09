#pragma once

#include "context.hpp"
#include "graphics.hpp"
#include "ui_pipeline.hpp"

namespace ash::ui
{
enum class ui_style
{
    WINDOW_PADDING
};

class ui : public system_base
{
public:
    ui();

    virtual bool initialize(const dictionary& config) override;

    void window(std::string_view label);
    bool window_ex(std::string_view label);
    void window_root(std::string_view label);
    void window_pop();

    void text(std::string_view text);

    bool tree(std::string_view label, bool leaf = false);
    std::tuple<bool, bool> tree_ex(std::string_view label, bool leaf = false);
    void tree_pop();

    bool collapsing(std::string_view label);
    void texture(graphics::resource* texture, float width, float height);

    bool drag(std::string_view label, float& value, float speed = 0.1f);
    bool drag(std::string_view label, math::float3& value, float speed = 0.1f);

    void style(ui_style style, float x = 0.0f, float y = 0.0f);
    void style_pop();

    std::pair<std::uint32_t, std::uint32_t> window_size();
    bool any_item_active() const;

    void begin_frame();
    void end_frame();

private:
    void initialize_theme();
    void initialize_font_texture();

    graphics::render_parameter* allocate_parameter();

    ecs::entity m_ui_entity;

    std::vector<std::unique_ptr<graphics::resource>> m_vertex_buffer;
    std::vector<std::unique_ptr<graphics::resource>> m_index_buffer;

    std::unique_ptr<graphics::resource> m_font;

    std::size_t m_parameter_counter;
    std::vector<std::unique_ptr<graphics::render_parameter>> m_parameter_pool;

    std::deque<graphics::scissor_rect> m_scissor_rects;

    std::unique_ptr<ui_pass> m_pass;
    std::size_t m_frame_index;

    bool m_enable_mouse;
};
}; // namespace ash::ui
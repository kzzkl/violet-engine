#pragma once

#include "context.hpp"
#include "graphics.hpp"
#include "ui_pipeline.hpp"

namespace ash::ui
{
class ui : public core::system_base
{
public:
    ui();

    virtual bool initialize(const dictionary& config) override;

    void begin_frame();
    void end_frame();

    void window();

private:
    void initialize_theme();
    void initialize_font_texture();

    ecs::entity m_ui_entity;

    std::vector<std::unique_ptr<graphics::resource>> m_vertex_buffer;
    std::vector<std::unique_ptr<graphics::resource>> m_index_buffer;

    std::unique_ptr<graphics::render_parameter> m_parameter;
    std::unique_ptr<graphics::resource> m_font;

    std::deque<graphics::scissor_rect> m_scissor_rects;

    std::unique_ptr<ui_pipeline> m_pipeline;
    std::size_t m_frame_index;

    bool m_enable_mouse;
};
}; // namespace ash::ui
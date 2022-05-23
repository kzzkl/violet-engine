#pragma once

#include "context.hpp"
#include "element_tree.hpp"
#include "font.hpp"
#include "graphics.hpp"
#include "ui_pipeline.hpp"

namespace ash::ui
{
enum class ui_style
{
    WINDOW_PADDING
};

class ui : public core::system_base
{
public:
    ui();

    virtual bool initialize(const dictionary& config) override;

    void window(std::string_view label);
    bool window_ex(std::string_view label);
    void window_root(std::string_view label);
    void window_pop();

    void text(std::string_view text, const element_rect& rect);

    bool tree(std::string_view label, bool leaf = false);
    std::tuple<bool, bool> tree_ex(std::string_view label, bool leaf = false);
    void tree_pop();

    bool collapsing(std::string_view label);
    void texture(graphics::resource* texture, const element_rect& rect);

    void begin_frame();
    void end_frame();

private:
    graphics::pipeline_parameter* allocate_parameter();

    std::vector<std::unique_ptr<graphics::resource>> m_vertex_buffers;
    std::unique_ptr<graphics::resource> m_index_buffer;

    std::size_t m_parameter_counter;
    std::vector<std::unique_ptr<graphics::pipeline_parameter>> m_parameter_pool;

    std::unique_ptr<ui_pipeline> m_pipeline;

    std::unique_ptr<font> m_font;

    ecs::entity m_root;
    element_tree m_tree;
};
}; // namespace ash::ui
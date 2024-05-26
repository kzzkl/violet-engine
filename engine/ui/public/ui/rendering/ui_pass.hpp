#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class ui_draw_list;
class ui_pass : public rdg_render_pass
{
public:
    ui_pass();

    void add_draw_list(ui_draw_list* draw_list) { m_draw_lists.push_back(draw_list); }
    void clear_draw_list() noexcept { m_draw_lists.clear(); }

    virtual void execute(rhi_command* command, rdg_context* context) override;

private:
    rdg_pass_reference* m_render_target;

    std::vector<ui_draw_list*> m_draw_lists;
};
} // namespace violet
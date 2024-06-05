#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class ui_painter;
class ui_pass : public rdg_render_pass
{
public:
    static constexpr std::size_t reference_render_target{0};

public:
    ui_pass();

    void add_painter(ui_painter* painter) { m_painters.push_back(painter); }
    void clear_painter() noexcept { m_painters.clear(); }

    virtual void execute(rhi_command* command, rdg_context* context) override;

private:
    std::vector<ui_painter*> m_painters;
};
} // namespace violet
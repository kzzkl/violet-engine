#pragma once

#include "imgui.h"
#include "ui_impl.hpp"

namespace ash::ui
{
class ui_impl_imgui : public ui_impl
{
public:
    ui_impl_imgui(const ui_impl_desc& desc);

    virtual void begin_frame(std::uint32_t width, std::uint32_t height, float delta) override;
    virtual void end_frame() override;

    virtual void test();

private:
    std::vector<std::unique_ptr<graphics::resource>> m_vertex_buffer;
    std::vector<std::unique_ptr<graphics::resource>> m_index_buffer;

    std::unique_ptr<graphics::render_parameter> m_parameter;

    std::unique_ptr<graphics::resource> m_font;

    std::size_t m_index;

    bool m_enable_mouse;
};
} // namespace ash::ui
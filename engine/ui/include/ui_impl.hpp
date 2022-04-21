#pragma once

#include "graphics.hpp"
#include <cstdint>

namespace ash::ui
{
struct ui_impl_desc
{
    void* window_handle;
    graphics::graphics* graphics;
    core::event* event;
};

using ui_render_data = std::vector<graphics::render_unit>;

class ui_impl
{
public:
    virtual ~ui_impl() = default;

    virtual void begin_frame(std::uint32_t width, std::uint32_t height, float delta) = 0;
    virtual void end_frame() = 0;

    const ui_render_data& render_data() const noexcept { return m_data; }
    virtual void test() {}

protected:
    ui_render_data m_data;
};
} // namespace ash::ui
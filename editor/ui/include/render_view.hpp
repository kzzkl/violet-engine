#pragma once

#include "editor_component.hpp"

namespace ash::editor
{
class render_view : public editor_view
{
public:
    virtual ~render_view() = default;

    virtual void draw(ui::ui& ui, editor_data& data) override;

private:
    void resize_target();

    std::uint32_t m_target_width;
    std::uint32_t m_target_height;

    bool m_resize_flag;

    std::unique_ptr<graphics::resource> m_target;
};
} // namespace ash::editor
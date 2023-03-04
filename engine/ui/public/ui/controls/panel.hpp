#pragma once

#include "ui/color.hpp"
#include "ui/control.hpp"

namespace violet::ui
{
class panel : public control
{
public:
    panel(std::uint32_t color = COLOR_WHITE, bool scissor = false);

    void color(std::uint32_t color) noexcept;

    virtual const control_mesh* mesh() const noexcept override { return &m_mesh; }

protected:
    virtual void on_extent_change(float width, float height) override;

private:
    std::array<math::float2, 4> m_position;
    std::array<math::float2, 4> m_uv;
    std::array<std::uint32_t, 4> m_color;
    std::array<std::uint32_t, 6> m_indices;

    control_mesh m_mesh;
};
} // namespace violet::ui
#pragma once

#include "ui/control.hpp"

namespace violet::ui
{
class image : public control
{
public:
    image(graphics::resource_interface* texture = nullptr);

    void texture(graphics::resource_interface* texture, bool resize = false);

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
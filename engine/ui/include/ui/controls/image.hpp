#pragma once

#include "ui/element.hpp"

namespace ash::ui
{
class image : public element
{
public:
    image(graphics::resource_interface* texture = nullptr);

    void texture(graphics::resource_interface* texture, bool resize = false);

    virtual const element_mesh* mesh() const noexcept override { return &m_mesh; }

protected:
    virtual void on_extent_change(float width, float height) override;

private:
    std::array<math::float2, 4> m_position;
    std::array<math::float2, 4> m_uv;
    std::array<std::uint32_t, 4> m_color;
    std::array<std::uint32_t, 6> m_indices;

    element_mesh m_mesh;
};
} // namespace ash::ui
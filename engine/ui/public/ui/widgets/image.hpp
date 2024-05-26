#pragma once

#include "ui/control.hpp"

namespace violet
{
class image : public control
{
public:
    image(rhi_texture* texture = nullptr);

    void texture(rhi_texture* texture, bool resize = false);

    virtual const control_mesh* mesh() const noexcept override { return &m_mesh; }

protected:
    virtual void on_extent_change(float width, float height) override;

private:
    std::array<float2, 4> m_position;
    std::array<float2, 4> m_uv;
    std::array<std::uint32_t, 4> m_color;
    std::array<std::uint32_t, 6> m_indices;

    control_mesh m_mesh;
};
} // namespace violet
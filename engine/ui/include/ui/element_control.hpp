#pragma once

#include "element_extent.hpp"
#include "graphics_interface.hpp"
#include "math/math.hpp"
#include <vector>

namespace ash::ui
{
enum element_control_type
{
    ELEMENT_CONTROL_TYPE_CONTAINER,
    ELEMENT_CONTROL_TYPE_BLOCK,
    ELEMENT_CONTROL_TYPE_TEXT,
    ELEMENT_CONTROL_TYPE_IMAGE
};

struct element_mesh
{
    std::vector<math::float2> vertex_position;
    std::vector<math::float2> vertex_uv;
    std::vector<std::uint32_t> vertex_color;
    std::vector<std::uint32_t> indices;

    graphics::resource* texture;
};

class element_control
{
public:
    virtual ~element_control() = default;

    virtual void extent(const element_extent& extent) = 0;

    element_control_type type() const noexcept { return m_type; }
    const element_mesh mesh() const noexcept { return m_mesh; }

protected:
    element_control_type m_type;
    element_mesh m_mesh;
};
} // namespace ash::ui
#pragma once

#include "graphics/render_graph/render_pass.hpp"
#include "math/math.hpp"

namespace violet
{
class camera
{
public:
    void set_perspective(float fov, float near_z, float far_z);
    void set_orthographic(float width, float near_z, float far_z);

private:
    float4x4 m_projection;
};
} // namespace violet
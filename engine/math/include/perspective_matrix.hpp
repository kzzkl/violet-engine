#pragma once

#include "matrix.hpp"

namespace ash::math
{
struct perspective_matrix : matrix
{
    template <matrix_4x4 M>
    inline static M make(float fov, float aspect, float zn, float zf)
    {
        float h = 1.0f / tanf(fov * 0.5f); // view space height
        float w = h / aspect;              // view space width
        return {w, 0, 0, 0, 0, h, 0, 0, 0, 0, zf / (zf - zn), 1, 0, 0, zn * zf / (zn - zf), 0};
    }
};
} // namespace ash::math
#pragma once

#include "matrix.hpp"

namespace ash::math
{
struct rotation_matrix : matrix
{
    template <matrix_4x4 M, vector_1x3_1x4 V>
    inline static M make_axis(const V& axis, float angle)
    {
        // TODO
    }
};
} // namespace ash::math
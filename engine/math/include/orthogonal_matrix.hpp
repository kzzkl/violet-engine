#pragma once

#include "matrix.hpp"

namespace ash::math
{
struct orthogonal_matrix : matrix
{
    template <square_matrix M>
    inline static M inverse(const M& m)
    {
        return matrix::transpose<M>(m);
    }
};
} // namespace ash::math
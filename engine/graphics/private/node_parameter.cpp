#include "graphics/node_parameter.hpp"

namespace violet
{
node_parameter::node_parameter() : pipeline_parameter(node_parameter::layout)
{
}

void node_parameter::set_world_matrix(const float4x4& matrix)
{
    float4x4 t = matrix::transpose(matrix);
    set(0, &t, sizeof(float4x4));
}
} // namespace violet
#include "graphics/render_graph/node_parameter.hpp"

namespace violet
{
node_parameter::node_parameter() : pipeline_parameter(node_parameter::layout)
{
}

void node_parameter::set_world_matrix(const float4x4& matrix)
{
    get_field<float4x4>(0) = matrix::transpose(matrix);
}
} // namespace violet
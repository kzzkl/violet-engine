#include "graphics/mesh_render.hpp"

namespace violet::graphics
{
object_pipeline_parameter::object_pipeline_parameter() : pipeline_parameter(object_pipeline_parameter::layout)
{
}

void object_pipeline_parameter::world_matrix(const math::float4x4& matrix)
{
    field<math::float4x4>(0) = math::matrix::transpose(matrix);
}
} // namespace violet::graphics
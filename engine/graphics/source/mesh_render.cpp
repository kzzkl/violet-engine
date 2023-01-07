#include "graphics/mesh_render.hpp"

namespace violet::graphics
{
object_pipeline_parameter::object_pipeline_parameter() : pipeline_parameter("violet_object")
{
}

void object_pipeline_parameter::world_matrix(const math::float4x4& matrix)
{
    field<math::float4x4>(0) = math::matrix::transpose(matrix);
}

std::vector<pipeline_parameter_pair> object_pipeline_parameter::layout()
{
    return {
        {PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(math::float4x4)}  // transform_m
    };
}
} // namespace violet::graphics
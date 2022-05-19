#pragma once

#include "pipeline_parameter.hpp"
#include <vector>

namespace ash::graphics
{
class skin_pipeline;
struct skinned_mesh
{
    std::vector<resource*> vertex_buffers;
    std::vector<std::unique_ptr<resource>> skinned_vertex_buffers;

    skin_pipeline* pipeline;
    std::unique_ptr<pipeline_parameter> parameter;

    std::size_t vertex_count;
};
} // namespace ash::graphics
#pragma once

#include "pipeline_parameter.hpp"
#include <memory>
#include <vector>

namespace violet::graphics
{
class skinning_pipeline;
struct skinned_mesh
{
    std::vector<std::unique_ptr<resource_interface>> skinned_vertex_buffers;

    skinning_pipeline* pipeline;
    std::unique_ptr<pipeline_parameter> parameter;

    std::size_t vertex_count;
};
} // namespace violet::graphics
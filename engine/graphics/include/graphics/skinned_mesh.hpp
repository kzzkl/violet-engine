#pragma once

#include "graphics_interface.hpp"
#include <memory>
#include <vector>

namespace ash::graphics
{
class skin_pipeline;
struct skinned_mesh
{
    std::vector<resource_interface*> input_vertex_buffers;
    std::vector<std::unique_ptr<resource_interface>> skinned_vertex_buffers;

    skin_pipeline* pipeline;
    std::unique_ptr<pipeline_parameter_interface> parameter;

    std::size_t vertex_count;
};
} // namespace ash::graphics
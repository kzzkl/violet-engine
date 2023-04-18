#pragma once

#include "graphics/rhi/node_parameter.hpp"
#include "graphics/rhi/render_pipeline.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace violet
{
struct material
{
    render_pipeline* pipeline;
    pipeline_parameter_interface* parameter;
};

struct submesh
{
    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;
};

struct mesh
{
    std::unordered_map<std::string, resource_interface*> vertex_buffers;
    resource_interface* index_buffer;

    std::vector<submesh> submeshes;
    std::vector<material> materials;

    std::unique_ptr<node_parameter> node_parameter;
};
} // namespace violet
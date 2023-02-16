#pragma once

#include "graphics/material.hpp"
#include "graphics/pipeline.hpp"
#include "graphics/render_group.hpp"

namespace violet::graphics
{
struct submesh
{
    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;
};

class object_pipeline_parameter : public pipeline_parameter
{
public:
    static constexpr pipeline_parameter_desc layout = {
        .parameters = {{PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER, sizeof(math::float4x4)}},
        .parameter_count = 1};

public:
    object_pipeline_parameter();

    void world_matrix(const math::float4x4& matrix);
};

struct mesh_render
{
    std::vector<resource_interface*> vertex_buffers;
    resource_interface* index_buffer;

    std::vector<submesh> submeshes;
    std::vector<material> materials;

    std::unique_ptr<object_pipeline_parameter> object_parameter;

    render_groups render_groups{RENDER_GROUP_1};
};
} // namespace violet::graphics
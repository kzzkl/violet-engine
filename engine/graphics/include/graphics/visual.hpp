#pragma once

#include "graphics/material.hpp"
#include "graphics/pipeline_parameter.hpp"
#include <vector>

namespace ash::graphics
{
struct submesh
{
    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;
};

enum visual_groups : std::uint32_t
{
    VISUAL_GROUP_1 = 1,
    VISUAL_GROUP_2 = 1 << 1,
    VISUAL_GROUP_3 = 1 << 2,
    VISUAL_GROUP_4 = 1 << 3,
    VISUAL_GROUP_5 = 1 << 4,
    VISUAL_GROUP_6 = 1 << 5,
    VISUAL_GROUP_7 = 1 << 6,
    VISUAL_GROUP_UI = 1 << 7,
    VISUAL_GROUP_DEBUG = 1 << 8,
    VISUAL_GROUP_EDITOR = 1 << 9
};

class object_pipeline_parameter : public pipeline_parameter
{
public:
    object_pipeline_parameter();

    void world_matrix(const math::float4x4& matrix);

    static std::vector<pipeline_parameter_pair> layout();
};

struct visual
{
    std::vector<resource_interface*> vertex_buffers;
    resource_interface* index_buffer;

    std::vector<submesh> submeshes;
    std::vector<material> materials;

    std::unique_ptr<object_pipeline_parameter> object_parameter;
    visual_groups groups{VISUAL_GROUP_1};
};
} // namespace ash::graphics
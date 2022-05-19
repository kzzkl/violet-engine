#pragma once

#include "graphics_interface.hpp"

namespace ash::graphics
{
class render_pipeline;
struct material
{
    render_pipeline* pipeline;
    std::vector<pipeline_parameter*> parameters;
};

struct submesh
{
    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;
};

struct visual
{
    enum mask_type : std::uint32_t
    {
        GROUP_1 = 1,
        GROUP_2 = GROUP_1 << 1,
        GROUP_3 = GROUP_2 << 1,
        GROUP_4 = GROUP_3 << 1,
        GROUP_5 = GROUP_4 << 1,
        GROUP_6 = GROUP_5 << 1,
        GROUP_7 = GROUP_6 << 1,
        UI = GROUP_7 << 1,
        DEBUG = UI << 1,
        EDITOR = DEBUG << 1
    };

    std::vector<resource_interface*> vertex_buffers;
    resource_interface* index_buffer;

    std::vector<submesh> submeshes;
    std::vector<material> materials;

    pipeline_parameter* object;
    std::uint32_t mask{GROUP_1};
};
} // namespace ash::graphics
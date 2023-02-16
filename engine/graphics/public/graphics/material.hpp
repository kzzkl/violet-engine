#pragma once

#include "graphics_interface.hpp"

namespace violet::graphics
{
class render_pipeline;
struct material
{
    render_pipeline* pipeline;
    pipeline_parameter_interface* parameter;

    scissor_extent scissor;
};
} // namespace violet::graphics
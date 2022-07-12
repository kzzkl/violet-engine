#pragma once

#include "graphics_interface.hpp"
#include <vector>

namespace ash::graphics
{
class render_pipeline;
struct material
{
    render_pipeline* pipeline;
    std::vector<pipeline_parameter_interface*> parameters;

    scissor_extent scissor;
};
} // namespace ash::graphics
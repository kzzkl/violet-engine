#pragma once

#include "graphics/pipeline_parameter.hpp"

namespace ash::graphics
{
class render_pipeline;
struct material
{
    render_pipeline* pipeline;
    std::vector<pipeline_parameter*> parameters;

    scissor_extent scissor;
};
} // namespace ash::graphics
#pragma once

#include "assert.hpp"
#include "component.hpp"
#include "render_pipeline.hpp"

namespace ash::graphics
{
struct visual
{
    std::vector<render_unit> submesh;
    render_parameter* object;
    std::uint32_t mask{1};
};
} // namespace ash::graphics
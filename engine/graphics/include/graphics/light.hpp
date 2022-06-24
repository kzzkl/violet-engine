#pragma once

#include "graphics/pipeline_parameter.hpp"
#include "math/math.hpp"

namespace ash::graphics
{
struct point_light
{
};

struct directional_light
{
    math::float3 direction;
    math::float3 color;
};

class light_pipeline_parameter : public pipeline_parameter
{
public:
private:
    struct constant_data
    {
    };
};
} // namespace ash::graphics
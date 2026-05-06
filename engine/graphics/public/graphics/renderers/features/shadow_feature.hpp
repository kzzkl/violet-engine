#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
enum shadow_sample_mode
{
    SHADOW_SAMPLE_MODE_NONE,
    SHADOW_SAMPLE_MODE_PCF,
    SHADOW_SAMPLE_MODE_PCSS,
};

class shadow_feature : public render_feature<shadow_feature>
{
public:
    shadow_sample_mode sample_mode{SHADOW_SAMPLE_MODE_PCF};
    std::uint32_t sample_count{8};
    float sample_radius{0.01f};

    float slope_scale_depth_bias{0.5f};
    float normal_bias{1.0f};
    float constant_bias{0.0f};
};
} // namespace violet
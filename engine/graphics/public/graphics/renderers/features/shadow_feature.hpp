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
    void set_sample_mode(shadow_sample_mode sample_mode) noexcept
    {
        m_sample_mode = sample_mode;
    }

    shadow_sample_mode get_sample_mode() const noexcept
    {
        return m_sample_mode;
    }

    void set_sample_count(std::uint32_t count) noexcept
    {
        m_sample_count = count;
    }

    std::uint32_t get_sample_count() const noexcept
    {
        return m_sample_count;
    }

    void set_sample_radius(float radius) noexcept
    {
        m_sample_radius = radius;
    }

    float get_sample_radius() const noexcept
    {
        return m_sample_radius;
    }

    void set_slope_scale_depth_bias(float bias) noexcept
    {
        m_slope_scale_depth_bias = bias;
    }

    float get_slope_scale_depth_bias() const noexcept
    {
        return m_slope_scale_depth_bias;
    }

    void set_normal_bias(float bias) noexcept
    {
        m_normal_bias = bias;
    }

    float get_normal_bias() const noexcept
    {
        return m_normal_bias;
    }

    void set_constant_bias(float bias) noexcept
    {
        m_constant_bias = bias;
    }

    float get_constant_bias() const noexcept
    {
        return m_constant_bias;
    }

private:
    shadow_sample_mode m_sample_mode{SHADOW_SAMPLE_MODE_PCF};
    std::uint32_t m_sample_count{8};
    float m_sample_radius{0.005f};

    float m_slope_scale_depth_bias{0.5f};
    float m_normal_bias{1.0f};
    float m_constant_bias{1.0f};
};
} // namespace violet
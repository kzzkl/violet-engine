#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class bloom_render_feature : public render_feature<bloom_render_feature>
{
public:
    void set_threshold(float threshold) noexcept
    {
        m_threshold = threshold;
    }

    float get_threshold() const noexcept
    {
        return m_threshold;
    }

    void set_intensity(float intensity) noexcept
    {
        m_intensity = intensity;
    }

    float get_intensity() const noexcept
    {
        return m_intensity;
    }

    void set_knee(float knee) noexcept
    {
        m_knee = knee;
    }

    float get_knee() const noexcept
    {
        return m_knee;
    }

    void set_radius(float radius) noexcept
    {
        m_radius = radius;
    }

    float get_radius() const noexcept
    {
        return m_radius;
    }

private:
    float m_threshold{0.9f};
    float m_intensity{0.1f};
    float m_knee{0.25f};
    float m_radius{0.5f};
};
} // namespace violet
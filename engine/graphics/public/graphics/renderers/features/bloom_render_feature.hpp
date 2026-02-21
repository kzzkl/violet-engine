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

private:
    float m_threshold{0.9f};
    float m_intensity{0.1f};
};
} // namespace violet
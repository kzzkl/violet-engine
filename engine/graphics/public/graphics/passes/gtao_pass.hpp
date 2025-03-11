#pragma once

#include "graphics/renderer.hpp"

namespace violet
{
class gtao_pass
{
public:
    struct parameter
    {
        std::uint32_t slice_count;
        std::uint32_t step_count;
        float radius;
        float falloff;

        rdg_texture* depth_buffer;
        rdg_texture* normal_buffer;
        rdg_texture* ao_buffer;
    };

    static void add(render_graph& graph, const parameter& parameter);
};

class gtao_render_feature : public render_feature<gtao_render_feature>
{
public:
    void set_slice_count(std::uint32_t slice_count) noexcept
    {
        m_slice_count = slice_count;
    }

    std::uint32_t get_slice_count() const noexcept
    {
        return m_slice_count;
    }

    void set_step_count(std::uint32_t step_count) noexcept
    {
        m_step_count = step_count;
    }

    std::uint32_t get_step_count() const noexcept
    {
        return m_step_count;
    }

    void set_radius(float radius) noexcept
    {
        m_radius = radius;
    }

    float get_radius() const noexcept
    {
        return m_radius;
    }

    void set_falloff(float falloff) noexcept
    {
        m_falloff = falloff;
    }

    float get_falloff() const noexcept
    {
        return m_falloff;
    }

private:
    std::uint32_t m_slice_count{3};
    std::uint32_t m_step_count{3};
    float m_radius{1.5f};
    float m_falloff{0.8f};
};
} // namespace violet
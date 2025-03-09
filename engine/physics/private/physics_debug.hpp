#pragma once

#include "physics/physics_interface.hpp"
#include <vector>

namespace violet
{
class physics_debug : public phy_debug_draw
{
public:
    struct line
    {
        vec3f start;
        vec3f end;
        vec3f color;
    };

    void draw_line(const vec3f& start, const vec3f& end, const vec3f& color) override
    {
        m_lines.emplace_back(line{
            .start = start,
            .end = end,
            .color = color,
        });
    }

    void reset()
    {
        m_lines.clear();
    }

    const std::vector<line>& get_lines() const noexcept
    {
        return m_lines;
    }

private:
    std::vector<line> m_lines;
};
} // namespace violet
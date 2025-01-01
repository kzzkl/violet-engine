#pragma once

#include "ecs/world.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "physics/physics_interface.hpp"

namespace violet
{
class physics_debug : public phy_debug_draw
{
public:
    physics_debug(world& world);

    void tick();

    void reset()
    {
        m_position.clear();
        m_color.clear();
        m_indexes.clear();
    }

    void draw_line(const vec3f& start, const vec3f& end, const vec3f& color) override
    {
        m_position.push_back(start);
        m_position.push_back(end);
        m_color.push_back(color);
        m_color.push_back(color);

        m_indexes.push_back(m_indexes.size());
        m_indexes.push_back(m_indexes.size());
    }

private:
    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<material> m_material;

    std::vector<vec3f> m_position;
    std::vector<vec3f> m_color;
    std::vector<std::uint32_t> m_indexes;

    entity m_object;
    world& m_world;
};
} // namespace violet
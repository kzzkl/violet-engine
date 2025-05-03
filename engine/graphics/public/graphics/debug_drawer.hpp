#pragma once

#include "ecs/world.hpp"
#include "graphics/geometry.hpp"
#include "graphics/material.hpp"
#include "math/box.hpp"

namespace violet
{
class debug_drawer
{
public:
    debug_drawer(world& world);

    void tick();

    void draw_line(const vec3f& start, const vec3f& end, const vec3f& color)
    {
        m_position.push_back(start);
        m_position.push_back(end);
        m_color.push_back(color);
        m_color.push_back(color);

        m_indexes.push_back(static_cast<std::uint32_t>(m_indexes.size()));
        m_indexes.push_back(static_cast<std::uint32_t>(m_indexes.size()));
    }

    void draw_box(const box3f& box, const vec3f& color)
    {
        vec3f corners[8] = {
            {box.min.x, box.min.y, box.min.z},
            {box.min.x, box.max.y, box.min.z},
            {box.min.x, box.min.y, box.max.z},
            {box.min.x, box.max.y, box.max.z},
            {box.max.x, box.min.y, box.min.z},
            {box.max.x, box.max.y, box.min.z},
            {box.max.x, box.min.y, box.max.z},
            {box.max.x, box.max.y, box.max.z},
        };

        draw_line(corners[0], corners[1], color);
        draw_line(corners[1], corners[3], color);
        draw_line(corners[2], corners[3], color);
        draw_line(corners[0], corners[2], color);
        draw_line(corners[4], corners[5], color);
        draw_line(corners[5], corners[7], color);
        draw_line(corners[6], corners[7], color);
        draw_line(corners[4], corners[6], color);
        draw_line(corners[1], corners[5], color);
        draw_line(corners[3], corners[7], color);
        draw_line(corners[2], corners[6], color);
        draw_line(corners[0], corners[4], color);
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
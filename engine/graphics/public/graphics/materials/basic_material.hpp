#pragma once

#include "graphics/material.hpp"
#include "math/math.hpp"

namespace violet
{
class basic_material : public material
{
public:
    struct data
    {
        float3 color;
    };

public:
    basic_material(const float3& color = {1.0f, 1.0f, 1.0f});

    void set_color(const float3& color);
    const float3& get_color() const noexcept;

private:
    data m_data;
};
} // namespace violet
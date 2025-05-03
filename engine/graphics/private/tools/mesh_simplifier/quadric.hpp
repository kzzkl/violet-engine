#pragma once

#include "math/types.hpp"
#include <optional>

namespace violet
{
class quadric
{
public:
    quadric() = default;
    quadric(const vec3f& a, const vec3f& b, const vec3f& c);

    quadric& operator+=(const quadric& other) noexcept;
    quadric operator+(const quadric& other) const noexcept;

    std::optional<vec3f> get_optimize_position() const noexcept;
    float get_error(const vec3f& position) const noexcept;

private:
    float m_xx;
    float m_xy;
    float m_xz;
    float m_yz;
    float m_yy;
    float m_zz;

    float m_xd;
    float m_yd;
    float m_zd;
    float m_dd;
};
} // namespace violet
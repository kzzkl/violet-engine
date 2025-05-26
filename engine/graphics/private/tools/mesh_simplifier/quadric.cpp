#include "tools/mesh_simplifier/quadric.hpp"
#include "math/matrix.hpp"
#include "math/vector.hpp"
#include <cassert>

namespace violet
{
quadric::quadric(const vec3f& a, const vec3f& b, const vec3f& c)
{
    vec3f ab = b - a;
    vec3f ac = c - a;

    vec3f n = vector::normalize(vector::cross(ab, ac));

    m_xx = n.x * n.x;
    m_xy = n.x * n.y;
    m_xz = n.x * n.z;
    m_yz = n.y * n.z;
    m_yy = n.y * n.y;
    m_zz = n.z * n.z;

    float d = -vector::dot(a, n);
    m_xd = n.x * d;
    m_yd = n.y * d;
    m_zd = n.z * d;
    m_dd = d * d;
}

quadric& quadric::operator+=(const quadric& other) noexcept
{
    m_xx += other.m_xx;
    m_xy += other.m_xy;
    m_xz += other.m_xz;
    m_yz += other.m_yz;
    m_yy += other.m_yy;
    m_zz += other.m_zz;

    m_xd += other.m_xd;
    m_yd += other.m_yd;
    m_zd += other.m_zd;
    m_dd += other.m_dd;

    return *this;
}

quadric quadric::operator+(const quadric& other) const noexcept
{
    quadric result = *this;
    result += other;
    return result;
}

std::optional<vec3f> quadric::get_optimize_position() const noexcept
{
    mat4f matrix = {
        vec4f{m_xx, m_xy, m_xz, m_xd},
        vec4f{m_xy, m_yy, m_yz, m_yd},
        vec4f{m_xz, m_yz, m_zz, m_zd},
        vec4f{m_xd, m_yd, m_zd, m_dd},
    };

    if (matrix::determinant(matrix) == 0)
    {
        return std::nullopt;
    }

    mat4f inverse_matrix = matrix::inverse(matrix);
    return vec3f{inverse_matrix[0][3], inverse_matrix[1][3], inverse_matrix[2][3]};
}

float quadric::get_error(const vec3f& position) const noexcept
{
    float error = 0.0f;
    error += m_xx * position.x * position.x + m_xy * position.y * position.x +
             m_xz * position.z * position.x + m_xd * position.x;
    error += m_xy * position.x * position.y + m_yy * position.y * position.y +
             m_yz * position.z * position.y + m_yd * position.y;
    error += m_xz * position.x * position.z + m_yz * position.y * position.z +
             m_zz * position.z * position.z + m_zd * position.z;
    error += m_xd * position.x + m_yd * position.y + m_zd * position.z + m_dd;

    return std::max(error, 0.0f);
}
} // namespace violet
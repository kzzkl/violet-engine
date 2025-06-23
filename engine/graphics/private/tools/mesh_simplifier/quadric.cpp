#include "tools/mesh_simplifier/quadric.hpp"
#include "math/matrix.hpp"
#include "math/vector.hpp"
#include <cassert>

namespace violet
{
void quadric::set(
    const vec3f& p0,
    const vec3f& p1,
    const vec3f& p2,
    const float* a0,
    const float* a1,
    const float* a2,
    std::uint32_t attribute_count)
{
    vec3f v01 = p1 - p0;
    vec3f v02 = p2 - p0;

    vec3f n = vector::cross(v02, v01);
    float length = vector::length(n);
    if (length < 1e-12f)
    {
        n = {0.0f, 0.0f, 0.0f};
    }
    else
    {
        n = n / length;
    }

    m_nn.xx = n.x * n.x;
    m_nn.xy = n.x * n.y;
    m_nn.xz = n.x * n.z;
    m_nn.yy = n.y * n.y;
    m_nn.yz = n.y * n.z;
    m_nn.zz = n.z * n.z;

    float distance = -vector::dot(p0, n);
    m_dn = n * distance;
    m_dd = distance * distance;

    mat3f m = {
        {v01.x, v01.y, v01.z},
        {v02.x, v02.y, v02.z},
        {n.x, n.y, n.z},
    };

    bool invertable;
    m = matrix::inverse(m, invertable);

    auto* g = get_g(attribute_count);
    auto* d = get_d(attribute_count);

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        if (invertable)
        {
            vec3f s = {a1[i] - a0[i], a2[i] - a0[i], 0.0f};
            g[i] = matrix::mul(m, s);
        }
        else
        {
            g[i] = {0.0f, 0.0f, 0.0f};
        }

        d[i] = a0[i] - vector::dot(g[i], p0);

        m_nn.xx += g[i].x * g[i].x;
        m_nn.xy += g[i].x * g[i].y;
        m_nn.xz += g[i].x * g[i].z;
        m_nn.yy += g[i].y * g[i].y;
        m_nn.yz += g[i].y * g[i].z;
        m_nn.zz += g[i].z * g[i].z;
        m_dn += d[i] * g[i];
        m_dd += d[i] * d[i];
    }
}

void quadric::set(const quadric& other, std::uint32_t attribute_count)
{
    m_nn = other.m_nn;
    m_dn = other.m_dn;
    m_dd = other.m_dd;

    auto* g = get_g(attribute_count);
    auto* d = get_d(attribute_count);
    const auto* other_g = other.get_g(attribute_count);
    const auto* other_d = other.get_d(attribute_count);

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        g[i] = other_g[i];
        d[i] = other_d[i];
    }
}

void quadric::add(const quadric& other, std::uint32_t attribute_count)
{
    m_nn += other.m_nn;
    m_dn += other.m_dn;
    m_dd += other.m_dd;

    auto* g = get_g(attribute_count);
    auto* d = get_d(attribute_count);
    const auto* other_g = other.get_g(attribute_count);
    const auto* other_d = other.get_d(attribute_count);

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        g[i] += other_g[i];
        d[i] += other_d[i];
    }
}

float quadric::evaluate(const vec3f& position, float* attributes, std::uint32_t attribute_count)
    const
{
    // Q(v) = vt*A*v + 2*bt*v + c
    float error = vector::dot(position, matrix::mul(static_cast<mat3f>(m_nn), position)) +
                  2.0f * vector::dot(position, m_dn) + m_dd;

    const auto* g = get_g(attribute_count);
    const auto* d = get_d(attribute_count);

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        float a = vector::dot(g[i], position) + d[i];
        error -= a * a;

        if (attributes != nullptr)
        {
            attributes[i] = a;
        }
    }

    return std::max(error, 0.0f);
}

void quadric_optimizer::add(const quadric& quadric, std::uint32_t attribute_count)
{
    m_nn += quadric.m_nn;
    m_dn += quadric.m_dn;

    const auto* g = quadric.get_g(attribute_count);
    const auto* d = quadric.get_d(attribute_count);

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        m_bb.xx += g[i].x * g[i].x;
        m_bb.xy += g[i].x * g[i].y;
        m_bb.xz += g[i].x * g[i].z;
        m_bb.yy += g[i].y * g[i].y;
        m_bb.yz += g[i].y * g[i].z;
        m_bb.zz += g[i].z * g[i].z;
        m_bd += g[i] * d[i];
    }
}

std::optional<vec3f> quadric_optimizer::optimize() const
{
    // ( C - 1/a * B*Bt ) * p = -1/a * B*d - dn

    symmetric_matrix m = m_nn - m_bb;
    vec3f b = m_bd - m_dn;

    bool invertable;
    mat3f m_inv = matrix::inverse(static_cast<mat3f>(m), invertable);
    if (!invertable)
    {
        return std::nullopt;
    }

    return matrix::mul(m_inv, b);
}

quadric_pool::quadric_pool(std::size_t size, std::uint32_t attribute_count)
{
    resize(size, attribute_count);
}

void quadric_pool::resize(std::size_t size, std::uint32_t attribute_count)
{
    m_attribute_count = attribute_count;
    m_data.resize(size * get_quadric_size());
}
} // namespace violet
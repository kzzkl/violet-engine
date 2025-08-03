#include "tools/mesh_simplifier/quadric.hpp"
#include "math/matrix.hpp"
#include "math/vector.hpp"
#include <cassert>

namespace violet
{
void quadric_edge::set(const vec3f& p0, const vec3f& p1, float edge_weight)
{
    m_n = p1 - p0;

    float length = vector::length(m_n);
    if (length < 1e-12f)
    {
        m_n = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
    }
    else
    {
        m_n = m_n / length;
    }

    m_a = length * edge_weight;

    m_nn.xx = m_a - m_a * m_n.x * m_n.x;
    m_nn.yy = m_a - m_a * m_n.y * m_n.y;
    m_nn.zz = m_a - m_a * m_n.z * m_n.z;
    m_nn.xy = -m_a * m_n.x * m_n.y;
    m_nn.xz = -m_a * m_n.x * m_n.z;
    m_nn.yz = -m_a * m_n.y * m_n.z;
}

void quadric::set(
    const vec3f& p0,
    const vec3f& p1,
    const vec3f& p2,
    const float* a0,
    const float* a1,
    const float* a2,
    std::span<const float> attribute_weights)
{
    vec3f v01 = p1 - p0;
    vec3f v02 = p2 - p0;

    vec3f n = vector::cross(v02, v01);
    float length = vector::length(n);
    if (length < 1e-12f)
    {
        n = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
    }
    else
    {
        n = n / length;
    }
    m_a = length * 0.5f;

    m_nn.xx = n.x * n.x;
    m_nn.xy = n.x * n.y;
    m_nn.xz = n.x * n.z;
    m_nn.yy = n.y * n.y;
    m_nn.yz = n.y * n.z;
    m_nn.zz = n.z * n.z;

    float distance = -vector::dot(p0, n);
    m_dn = n * distance;
    m_dd = distance * distance;

    mat4f m = {
        {.x = p0.x, .y = p0.y, .z = p0.z, .w = 1.0f},
        {.x = p1.x, .y = p1.y, .z = p1.z, .w = 1.0f},
        {.x = p2.x, .y = p2.y, .z = p2.z, .w = 1.0f},
        {.x = n.x, .y = n.y, .z = n.z, .w = 0.0f},
    };

    float det;
    m = matrix::inverse(m, det);

    auto attribute_count = static_cast<std::uint32_t>(attribute_weights.size());

    auto* g = get_g(attribute_count);
    auto* d = get_d(attribute_count);

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        float s0 = a0[i] * attribute_weights[i];
        float s1 = a1[i] * attribute_weights[i];
        float s2 = a2[i] * attribute_weights[i];

        assert(!std::isnan(s0) && !std::isnan(s1) && !std::isnan(s2));

        if (std::abs(det) > 1e-8f)
        {
            vec4f s = {s0, s1, s2, 0.0f};
            vec4f gd = matrix::mul(m, s);
            g[i] = gd;
            d[i] = gd.w;
        }
        else
        {
            g[i] = {.x = 0.0f, .y = 0.0f, .z = 0.0f};
            d[i] = s0;
        }

        m_nn.xx += g[i].x * g[i].x;
        m_nn.xy += g[i].x * g[i].y;
        m_nn.xz += g[i].x * g[i].z;
        m_nn.yy += g[i].y * g[i].y;
        m_nn.yz += g[i].y * g[i].z;
        m_nn.zz += g[i].z * g[i].z;
        m_dn += d[i] * g[i];
        m_dd += d[i] * d[i];
    }

    m_nn *= m_a;
    m_dn *= m_a;
    m_dd *= m_a;

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        g[i] *= m_a;
        d[i] *= m_a;
    }

    assert(!std::isnan(m_nn.xx));
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

    m_a = other.m_a;
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

    m_a += other.m_a;
}

void quadric::add(const quadric_edge& other, const vec3f& p0)
{
    m_nn += other.m_nn;

    float distance = -vector::dot(p0, other.m_n);
    m_dn += other.m_a * -p0 - other.m_a * distance * other.m_n;
    m_dd += other.m_a * vector::dot(p0, p0) - other.m_a * distance * distance;
}

float quadric::evaluate(
    const vec3f& position,
    float* attributes,
    std::span<const float> attribute_weights) const
{
    auto attribute_count = static_cast<std::uint32_t>(attribute_weights.size());

    // Q(v) = vt*A*v + 2*bt*v + c
    float error = vector::dot(position, matrix::mul(static_cast<mat3f>(m_nn), position)) +
                  (2.0f * vector::dot(position, m_dn)) + m_dd;

    const auto* g = get_g(attribute_count);
    const auto* d = get_d(attribute_count);

    for (std::size_t i = 0; i < attribute_count; ++i)
    {
        float pgd = vector::dot(g[i], position) + d[i];
        float s = pgd / m_a;
        error -= pgd * s;

        if (attributes != nullptr)
        {
            attributes[i] = s / attribute_weights[i];
            assert(!std::isnan(attributes[i]));
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

void quadric_optimizer::add(const quadric& quadric)
{
    m_nn += quadric.m_nn;
    m_dn += quadric.m_dn;
}

std::optional<vec3f> quadric_optimizer::optimize() const
{
    // ( C - 1/a * B*Bt ) * p = -1/a * B*d - dn

    symmetric_matrix m = m_nn - m_bb;
    vec3f b = m_bd - m_dn;

    float det;
    mat3f m_inv = matrix::inverse(static_cast<mat3f>(m), det);
    if (det < 1e-8f)
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
#pragma once

#include "math/types.hpp"
#include <optional>
#include <vector>

namespace violet
{
struct symmetric_matrix
{
    float xx;
    float xy;
    float xz;
    float yz;
    float yy;
    float zz;

    symmetric_matrix& operator+=(const symmetric_matrix& other) noexcept
    {
        xx += other.xx;
        xy += other.xy;
        xz += other.xz;
        yz += other.yz;
        yy += other.yy;
        zz += other.zz;
        return *this;
    }

    symmetric_matrix operator+(const symmetric_matrix& other) const noexcept
    {
        symmetric_matrix result = *this;
        result += other;
        return result;
    }

    symmetric_matrix operator-=(const symmetric_matrix& other) noexcept
    {
        xx -= other.xx;
        xy -= other.xy;
        xz -= other.xz;
        yz -= other.yz;
        yy -= other.yy;
        zz -= other.zz;
        return *this;
    }

    symmetric_matrix operator-(const symmetric_matrix& other) const noexcept
    {
        symmetric_matrix result = *this;
        result -= other;
        return result;
    }

    operator mat3f() const noexcept
    {
        return {
            {xx, xy, xz},
            {xy, yy, yz},
            {xz, yz, zz},
        };
    }
};

class quadric
{
public:
    void set(
        const vec3f& p0,
        const vec3f& p1,
        const vec3f& p2,
        const float* a0,
        const float* a1,
        const float* a2,
        std::uint32_t attribute_count);
    void set(const quadric& other, std::uint32_t attribute_count);

    void add(const quadric& other, std::uint32_t attribute_count);

    float evaluate(const vec3f& position, float* attributes, std::uint32_t attribute_count) const;

private:
    vec3f* get_g(std::uint32_t attribute_count) noexcept
    {
        return reinterpret_cast<vec3f*>(this + 1);
    }

    const vec3f* get_g(std::uint32_t attribute_count) const noexcept
    {
        return reinterpret_cast<const vec3f*>(this + 1);
    }

    float* get_d(std::uint32_t attribute_count) noexcept
    {
        return reinterpret_cast<float*>(get_g(attribute_count) + attribute_count);
    }

    const float* get_d(std::uint32_t attribute_count) const noexcept
    {
        return reinterpret_cast<const float*>(get_g(attribute_count) + attribute_count);
    }

    symmetric_matrix m_nn;
    vec3f m_dn;
    float m_dd;

    friend class quadric_optimizer;
};

template <std::uint32_t attribute_count>
class quadric_attribute : public quadric
{
private:
    std::array<vec3f, attribute_count> m_g;
    std::array<float, attribute_count> m_d;
};

class quadric_optimizer
{
public:
    void add(const quadric& quadric, std::uint32_t attribute_count);

    std::optional<vec3f> optimize() const;

private:
    symmetric_matrix m_nn; // C - 1/a * B*Bt
    vec3f m_dn;            // -1/a * B*d - dn

    symmetric_matrix m_bb;
    vec3f m_bd;
};

class quadric_pool
{
public:
    quadric_pool() = default;
    quadric_pool(std::size_t size, std::uint32_t attribute_count);

    quadric& operator[](std::size_t index)
    {
        std::size_t quadric_size = get_quadric_size();
        return *reinterpret_cast<quadric*>(m_data.data() + quadric_size * index);
    }

    void resize(std::size_t size, std::uint32_t attribute_count);

    std::size_t get_size() const noexcept
    {
        return m_data.size() / get_quadric_size();
    }

private:
    std::size_t get_quadric_size() const noexcept
    {
        return sizeof(quadric) + m_attribute_count * sizeof(vec4f);
    }

    std::vector<float> m_data;
    std::uint32_t m_attribute_count;
};
} // namespace violet
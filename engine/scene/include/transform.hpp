#pragma once

#include "component.hpp"
#include "math.hpp"
#include "scene_exports.hpp"
#include "scene_node.hpp"

namespace ash::scene
{
class SCENE_API transform
{
public:
    transform();
    transform(transform&& other) noexcept;

    transform& operator=(transform&& other) noexcept;

    const math::float3& position() const noexcept { return m_position; }
    void position(float x, float y, float z) noexcept
    {
        m_position = {x, y, z};
        m_node->mark_dirty();
    }
    void position(const math::float3& p) noexcept
    {
        m_position = p;
        m_node->mark_dirty();
    }
    void position(const math::float4_simd& p) noexcept
    {
        math::simd::store(p, m_position);
        m_node->mark_dirty();
    }

    const math::float4& rotation() const noexcept { return m_rotation; }
    void rotation(float x, float y, float z, float w) noexcept
    {
        m_rotation = {x, y, z, w};
        m_node->mark_dirty();
    }
    void rotation(const math::float4& r) noexcept
    {
        m_rotation = r;
        m_node->mark_dirty();
    }
    void rotation(const math::float4_simd& r) noexcept
    {
        math::simd::store(r, m_rotation);
        m_node->mark_dirty();
    }

    const math::float3& scaling() const noexcept { return m_scaling; }
    void scaling(float x, float y, float z) noexcept
    {
        m_scaling = {x, y, z};
        m_node->mark_dirty();
    }
    void scaling(const math::float3& s) noexcept
    {
        m_scaling = s;
        m_node->mark_dirty();
    }
    void scaling(const math::float4_simd& s) noexcept
    {
        math::simd::store(s, m_scaling);
        m_node->mark_dirty();
    }

    scene_node* node() const noexcept { return m_node.get(); }

private:
    math::float3 m_position;
    math::float4 m_rotation;
    math::float3 m_scaling;

    std::unique_ptr<scene_node> m_node;
};
} // namespace ash::scene

namespace ash::ecs
{
template <>
struct component_trait<scene::transform>
{
    static constexpr std::size_t id = uuid("a439dd80-979b-4bf9-a540-43eeef9e556d").hash();
};
} // namespace ash::ecs

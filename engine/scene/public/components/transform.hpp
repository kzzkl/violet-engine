#pragma once

#include "core/ecs/actor.hpp"
#include "math/math.hpp"
#include <vector>

namespace violet
{
class transform
{
public:
    transform(actor* owner) noexcept;

    void set_position(float x, float y, float z) noexcept;
    void set_position(const float3& position) noexcept;
    void set_position(float4_simd position) noexcept;
    const float3& get_position() const noexcept;
    float3 get_world_position() const noexcept;

    void set_rotation(const float4& quaternion) noexcept;
    void set_rotation(float4_simd quaternion) noexcept;
    void set_rotation_euler(const float3& euler) noexcept;
    const float4& get_rotation() const noexcept;

    void set_scale(float x, float y, float z) noexcept;
    void set_scale(const float3& value) noexcept;
    void set_scale(float4_simd value) noexcept;
    const float3& get_scale() const noexcept;

    float3 get_up() const noexcept;

    void lookat(const float3& target, const float3& up) noexcept;

    void set_world_matrix(const float4x4& matrix);
    void set_world_matrix(const float4x4_simd& matrix);

    const float4x4& get_local_matrix() const noexcept;
    const float4x4& get_world_matrix() const noexcept;

    void set_parent(const component_ptr<transform>& parent) noexcept { m_parent = parent; }
    component_ptr<transform> get_parent() const noexcept { return m_parent; }

    void add_child(const component_ptr<transform>& child);
    void remove_child(const component_ptr<transform>& child);

    template <typename Functor>
    void each_children(Functor functor)
    {
        for (auto& child : m_children)
            functor(&(*child));
    }

    template <typename Functor>
    void bfs(Functor functor)
    {
        std::queue<transform*> bfs;
        bfs.push(this);

        while (!bfs.empty())
        {
            transform* node = bfs.front();
            bfs.pop();

            if (functor(*node))
            {
                for (auto& child : node->m_children)
                    bfs.push(&(*child));
            }
        }
    }

private:
    void update_local() noexcept;
    void mark_dirty();

    float3 m_position;
    float4 m_rotation;
    float3 m_scale;

    float4x4 m_local_matrix;
    mutable float4x4 m_world_matrix;
    mutable bool m_world_dirty;

    component_ptr<transform> m_parent;
    std::vector<component_ptr<transform>> m_children;

    actor* m_owner;
};
} // namespace violet
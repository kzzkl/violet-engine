#include "components/transform.hpp"

namespace violet
{
transform::transform(actor* owner) noexcept
    : m_position{0.0f, 0.0f, 0.0f},
      m_rotation{0.0f, 0.0f, 0.0f, 1.0f},
      m_scale{1.0f, 1.0f, 1.0f},
      m_local_matrix(matrix::identity<float4x4>()),
      m_world_matrix(matrix::identity<float4x4>()),
      m_world_dirty(false),
      m_owner(owner)
{
}

void transform::set_position(float x, float y, float z) noexcept
{
    m_position[0] = x;
    m_position[1] = y;
    m_position[2] = z;

    update_local();
    mark_dirty();
}

void transform::set_position(const float3& position) noexcept
{
    m_position = position;

    update_local();
    mark_dirty();
}

const float3& transform::get_position() const noexcept
{
    return m_position;
}

float3 transform::get_world_position() const noexcept
{
    auto& world = get_world_matrix();
    return float3{world[3][0], world[3][1], world[3][2]};
}

void transform::set_rotation(const float4& quaternion) noexcept
{
    m_rotation = quaternion;

    update_local();
    mark_dirty();
}

void transform::set_rotation_euler(const float3& euler) noexcept
{
    math::store(quaternion::from_euler(euler[0], euler[1], euler[2]), m_rotation);

    update_local();
    mark_dirty();
}

const float4& transform::get_rotation() const noexcept
{
    return m_rotation;
}

void transform::set_scale(float x, float y, float z) noexcept
{
    m_scale[0] = x;
    m_scale[1] = y;
    m_scale[2] = z;

    update_local();
    mark_dirty();
}

void transform::set_scale(const float3& value) noexcept
{
    m_scale = value;

    update_local();
    mark_dirty();
}

const float3& transform::get_scale() const noexcept
{
    return m_scale;
}

float3 transform::get_up() const noexcept
{
    vector4 up = vector::set(0.0f, 1.0f, 0.0f, 0.0f);
    up = quaternion::mul_vec(math::load(m_rotation), up);
    float3 result;
    math::store(up, result);
    return result;
}

void transform::lookat(const float3& target, const float3& up) noexcept
{
    vector4 t = math::load(target);
    vector4 p = math::load(m_position);
    vector4 u = math::load(up);

    vector4 z_axis = vector::normalize(vector::sub(t, p));
    vector4 x_axis = vector::normalize(vector::cross(u, z_axis));
    vector4 y_axis = vector::cross(z_axis, x_axis);

    matrix4 rotation = {x_axis, y_axis, z_axis, math::IDENTITY_ROW_3};
    math::store(quaternion::from_matrix(rotation), m_rotation);

    update_local();
    mark_dirty();
}

void transform::set_world_matrix(const float4x4& matrix)
{
    if (m_parent)
    {
        matrix4 parent_to_world = math::load(m_parent->get_world_matrix());
        matrix4 local_matrix =
            matrix::mul(math::load(matrix), matrix::inverse_transform(parent_to_world));

        vector4 s, r, t;
        matrix::decompose(local_matrix, s, r, t);

        math::store(local_matrix, m_local_matrix);
        math::store(s, m_scale);
        math::store(r, m_rotation);
        math::store(t, m_position);
    }
    else
    {
        m_local_matrix = matrix;
        m_world_matrix = matrix;

        matrix4 local_matrix = math::load(m_local_matrix);
        vector4 s, r, t;
        matrix::decompose(local_matrix, s, r, t);
        math::store(s, m_scale);
        math::store(r, m_rotation);
        math::store(t, m_position);
    }

    mark_dirty();
}

const float4x4& transform::get_local_matrix() const noexcept
{
    return m_local_matrix;
}

const float4x4& transform::get_world_matrix() const noexcept
{
    if (m_world_dirty)
    {
        const transform* node = this;
        std::vector<const transform*> path;
        do
        {
            path.push_back(node);
            node = node->m_parent.get();
        } while (node->m_world_dirty);

        for (auto iter = path.rbegin(); iter != path.rend(); ++iter)
        {
            math::store(
                matrix::mul(
                    math::load((*iter)->m_local_matrix),
                    math::load((*iter)->m_parent->m_world_matrix)),
                (*iter)->m_world_matrix);

            (*iter)->m_world_dirty = false;
        }
    }

    return m_world_matrix;
}

void transform::add_child(const component_ptr<transform>& child)
{
    child->m_world_dirty = true;
    m_children.push_back(child);

    if (child->m_parent)
        child->m_parent->remove_child(child);
    child->m_parent = m_owner->get<transform>();
}

void transform::remove_child(const component_ptr<transform>& child)
{
    for (auto iter = m_children.begin(); iter != m_children.end(); ++iter)
    {
        if (iter->get_owner() == child.get_owner())
        {
            std::swap(*iter, m_children.back());
            m_children.pop_back();
            break;
        }
    }
}

void transform::update_local() noexcept
{
    matrix4 local_matrix = matrix::affine_transform(
        math::load(m_scale),
        math::load(m_rotation),
        math::load(m_position));
    math::store(local_matrix, m_local_matrix);

    if (!m_parent)
        math::store(local_matrix, m_world_matrix);
}

void transform::mark_dirty()
{
    bfs(
        [](transform& node) -> bool
        {
            if (node.m_world_dirty)
            {
                return false;
            }
            else
            {
                node.m_world_dirty = true;
                return true;
            }
        });

    if (!m_parent)
        m_world_dirty = false;
}
} // namespace violet
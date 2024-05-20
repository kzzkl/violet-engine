#include "components/transform.hpp"

namespace violet
{
transform::transform(actor* owner) noexcept
    : m_position{0.0f, 0.0f, 0.0f},
      m_rotation{0.0f, 0.0f, 0.0f, 1.0f},
      m_scale{1.0f, 1.0f, 1.0f},
      m_local_matrix(matrix::identity()),
      m_world_matrix(matrix::identity()),
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

void transform::set_position(float4_simd position) noexcept
{
    simd::store(position, m_position);

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

void transform::set_rotation(float4_simd quaternion) noexcept
{
    simd::store(quaternion, m_rotation);

    update_local();
    mark_dirty();
}

void transform::set_rotation_euler(const float3& euler) noexcept
{
    m_rotation = quaternion::rotation_euler(euler);

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

void transform::set_scale(float4_simd value) noexcept
{
    simd::store(value, m_scale);

    update_local();
    mark_dirty();
}

const float3& transform::get_scale() const noexcept
{
    return m_scale;
}

float3 transform::get_up() const noexcept
{
    return quaternion::mul_vec(m_rotation, float3{0.0f, 1.0f, 0.0f});
}

void transform::lookat(const float3& target, const float3& up) noexcept
{
    float3 z_axis = vector::normalize(vector::sub(target, m_position));
    float3 x_axis = vector::normalize(vector::cross(up, z_axis));
    float3 y_axis = vector::cross(z_axis, x_axis);

    float4x4 rotation = {
        float4{x_axis[0], x_axis[1], x_axis[2], 0.0f},
        float4{y_axis[0], y_axis[1], y_axis[2], 0.0f},
        float4{z_axis[0], z_axis[1], z_axis[2], 0.0f},
        float4{0.0f,      0.0f,      0.0f,      1.0f}
    };

    m_rotation = quaternion::rotation_matrix(rotation);

    update_local();
    mark_dirty();
}

void transform::set_world_matrix(const float4x4& matrix)
{
    if (m_parent)
    {
        m_local_matrix =
            matrix::mul(matrix, matrix::inverse_transform(m_parent->get_world_matrix()));
    }
    else
    {
        m_local_matrix = matrix;
        m_world_matrix = matrix;
    }

    matrix::decompose(m_local_matrix, m_scale, m_rotation, m_position);
    mark_dirty();
}

void transform::set_world_matrix(const float4x4_simd& matrix)
{
    if (m_parent)
    {
        float4x4_simd parent_to_world = simd::load(m_parent->get_world_matrix());
        simd::store(
            matrix_simd::mul(matrix, matrix_simd::inverse_transform(parent_to_world)),
            m_local_matrix);
    }
    else
    {
        simd::store(matrix, m_local_matrix);
        simd::store(matrix, m_world_matrix);
    }

    matrix::decompose(m_local_matrix, m_scale, m_rotation, m_position);
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
            simd::store(
                matrix_simd::mul(
                    simd::load((*iter)->m_local_matrix),
                    simd::load((*iter)->m_parent->m_world_matrix)),
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
    float4x4_simd local_matrix = matrix_simd::affine_transform(
        simd::load(m_scale),
        simd::load(m_rotation),
        simd::load(m_position));
    simd::store(local_matrix, m_local_matrix);

    if (!m_parent)
        simd::store(local_matrix, m_world_matrix);
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
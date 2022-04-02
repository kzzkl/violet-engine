#include "transform.hpp"

namespace ash::scene
{
transform::transform()
    : m_position{0.0f, 0.0f, 0.0f},
      m_rotation{0.0f, 0.0f, 0.0f, 1.0f},
      m_scaling{1.0f, 1.0f, 1.0f},
      m_node(std::make_unique<scene_node>(this))
{
}

transform::transform(transform&& other) noexcept
{
    m_position = other.m_position;
    m_rotation = other.m_rotation;
    m_scaling = other.m_scaling;

    m_node = std::move(other.m_node);
    m_node->transform(this);
}

transform& transform::operator=(transform&& other) noexcept
{
    m_position = other.m_position;
    m_rotation = other.m_rotation;
    m_scaling = other.m_scaling;

    m_node = std::move(other.m_node);
    m_node->transform(this);

    return *this;
}
} // namespace ash::scene
#include "transform.hpp"
#include "log.hpp"

namespace ash::scene
{
transform::transform()
    : position{0.0f, 0.0f, 0.0f},
      rotation{0.0f, 0.0f, 0.0f, 1.0f},
      scaling{1.0f, 1.0f, 1.0f},
      parent_matrix(math::matrix_plain::identity()),
      world_matrix(math::matrix_plain::identity()),
      node(std::make_unique<transform_node>())
{
    node->transform = this;
    node->dirty = true;
    node->parent = nullptr;
}

transform::transform(transform&& other) noexcept
    : position(other.position),
      rotation(other.rotation),
      scaling(other.scaling)
{
    if (other.node)
    {
        node = std::move(other.node);
        node->transform = this;
    }
}

void transform::mark_write()
{
    node->dirty = true;
}

transform& transform::operator=(transform&& other) noexcept
{
    position = other.position;
    rotation = other.rotation;
    scaling = other.scaling;

    if (other.node)
    {
        node = std::move(other.node);
        node->transform = this;
    }

    return *this;
}
} // namespace ash::scene
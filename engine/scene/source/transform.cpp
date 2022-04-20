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
      dirty(true),
      sync_count(0),
      in_scene(false)
{
}
} // namespace ash::scene
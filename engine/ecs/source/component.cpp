#include "component.hpp"

namespace ash::ecs
{
namespace detail
{
component_index next_component_index()
{
    static component_index index = 0;
    return index++;
}
} // namespace detail
} // namespace ash::ecs
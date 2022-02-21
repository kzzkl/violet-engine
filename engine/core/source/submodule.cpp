#include "submodule.hpp"

namespace ash::core
{
namespace detail
{
submodule_index next_submodule_index()
{
    static submodule_index index = 0;
    return index++;
}
} // namespace detail

submodule::submodule(std::string_view name) : m_name(name)
{
}
} // namespace ash::core
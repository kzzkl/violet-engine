#include "submodule.hpp"

namespace ash::core
{
submodule::submodule(std::string_view name) noexcept : m_name(name)
{
}

context& submodule::get_context()
{
    return *m_context;
}
} // namespace ash::core
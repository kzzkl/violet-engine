#include "core/context/engine_module.hpp"

namespace violet::core
{
engine_module::engine_module(std::string_view name) noexcept : m_name(name)
{
}
} // namespace violet::core
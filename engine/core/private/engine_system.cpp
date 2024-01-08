#include "core/engine_system.hpp"
#include "engine_context.hpp"

namespace violet
{
engine_system::engine_system(std::string_view name) noexcept : m_name(name), m_context(nullptr)
{
}

engine_system* engine_system::get_system(std::size_t index)
{
    return m_context->get_system(index);
}

timer& engine_system::get_timer()
{
    return m_context->get_timer();
}

world& engine_system::get_world()
{
    return m_context->get_world();
}
} // namespace violet
#include "engine_context.hpp"
#include "common/log.hpp"
#include <fstream>

namespace violet
{
engine_context::engine_context() : m_exit(true)
{
    m_timer = std::make_unique<timer>();
    m_world = std::make_unique<world>();
}

engine_context::~engine_context()
{
}

void engine_context::set_system(std::size_t index, engine_system* system)
{
    if (index >= m_systems.size())
        m_systems.resize(index + 1);

    assert(m_systems[index] == nullptr);
    m_systems[index] = system;
}

engine_system* engine_context::get_system(std::size_t index)
{
    return m_systems[index];
}
} // namespace violet
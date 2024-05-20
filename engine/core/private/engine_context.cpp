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

void engine_context::set_module(std::size_t index, engine_module* module)
{
    if (index >= m_modules.size())
        m_modules.resize(index + 1);

    assert(m_modules[index] == nullptr);
    m_modules[index] = module;
}

engine_module* engine_context::get_module(std::size_t index)
{
    return m_modules[index];
}

void engine_context::tick(float delta)
{
    m_executor.execute_sync(m_frame_begin);
    m_executor.execute_sync(m_tick, delta);
    m_executor.execute_sync(m_frame_end);
}
} // namespace violet
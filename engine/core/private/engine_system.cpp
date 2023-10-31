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

task_executor& engine_system::get_task_executor()
{
    return m_context->get_task_executor();
}

task<>& engine_system::on_frame_begin()
{
    return m_context->get_frame_begin_task().get_root();
}

task<>& engine_system::on_frame_end()
{
    return m_context->get_frame_end_task().get_root();
}

task<float>& engine_system::on_tick()
{
    return m_context->get_tick_task().get_root();
}
} // namespace violet
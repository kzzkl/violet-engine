#include "core/engine_module.hpp"
#include "engine_context.hpp"

namespace violet
{
engine_module::engine_module(std::string_view name) noexcept : m_name(name), m_context(nullptr)
{
}

engine_module* engine_module::get_module(std::size_t index)
{
    return m_context->get_module(index);
}

timer& engine_module::get_timer()
{
    return m_context->get_timer();
}

world& engine_module::get_world()
{
    return m_context->get_world();
}

task<>& engine_module::on_frame_begin() noexcept
{
    return m_context->on_frame_begin();
}

task<>& engine_module::on_frame_end() noexcept
{
    return m_context->on_frame_end();
}

task<float>& engine_module::on_tick() noexcept
{
    return m_context->on_tick();
}

task_executor& engine_module::get_executor() noexcept
{
    return m_context->get_executor();
}
} // namespace violet
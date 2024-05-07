#pragma once

#include "core/ecs/world.hpp"
#include "core/engine_system.hpp"
#include "core/timer.hpp"
#include "task/task_executor.hpp"

namespace violet
{
class engine_context
{
public:
    engine_context();
    engine_context(const engine_context&) = delete;
    ~engine_context();

    void set_system(std::size_t index, engine_system* system);
    engine_system* get_system(std::size_t index);

    timer& get_timer() { return *m_timer; }
    world& get_world() { return *m_world; }

    task<>& on_frame_begin() noexcept { return m_frame_begin.get_root(); }
    task<>& on_frame_end() noexcept { return m_frame_end.get_root(); }
    task<float>& on_tick() noexcept { return m_tick.get_root(); }

    task_executor& get_executor() noexcept { return m_executor; }

    void tick(float delta);

    engine_context& operator=(const engine_context&) = delete;

private:
    std::vector<engine_system*> m_systems;

    std::unique_ptr<timer> m_timer;
    std::unique_ptr<world> m_world;

    task_graph<> m_frame_begin;
    task_graph<> m_frame_end;
    task_graph<float> m_tick;

    task_executor m_executor;

    std::atomic<bool> m_exit;
};
} // namespace violet
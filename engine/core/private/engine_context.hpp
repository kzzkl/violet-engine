#pragma once

#include "core/ecs/world.hpp"
#include "core/engine_system.hpp"
#include "core/task/task_executor.hpp"
#include "core/timer.hpp"

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

    task_executor& get_task_executor() { return *m_task_executor; }

    task_graph<>& get_frame_begin_task() { return m_frame_begin; }
    task_graph<>& get_frame_end_task() { return m_frame_end; }
    task_graph<float>& get_tick_task() { return m_tick; }

    engine_context& operator=(const engine_context&) = delete;

private:
    std::vector<engine_system*> m_systems;

    std::unique_ptr<timer> m_timer;
    std::unique_ptr<world> m_world;

    std::unique_ptr<task_executor> m_task_executor;

    task_graph<> m_frame_begin;
    task_graph<> m_frame_end;
    task_graph<float> m_tick;

    std::atomic<bool> m_exit;
};
} // namespace violet
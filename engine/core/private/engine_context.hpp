#pragma once

#include "core/engine.hpp"
#include "core/timer.hpp"
#include "ecs/world.hpp"
#include "task/task_executor.hpp"

namespace violet
{
class engine_context
{
public:
    engine_context();
    engine_context(const engine_context&) = delete;
    engine_context& operator=(const engine_context&) = delete;

    ~engine_context();

    void set_system(std::size_t index, system* system);

    system* get_system(std::size_t index) const
    {
        return m_systems[index];
    }

    bool has_system(std::size_t index) const noexcept
    {
        return index < m_systems.size() && m_systems[index] != nullptr;
    }

    timer& get_timer()
    {
        return *m_timer;
    }

    world& get_world()
    {
        return *m_world;
    }

    task_graph& get_task_graph() noexcept
    {
        return m_task_graph;
    }

    task_executor& get_task_executor() noexcept
    {
        return m_executor;
    }

private:
    std::vector<system*> m_systems;

    std::unique_ptr<timer> m_timer;
    std::unique_ptr<world> m_world;

    task_graph m_task_graph;
    task_executor m_executor;

    std::atomic<bool> m_exit;
};
} // namespace violet
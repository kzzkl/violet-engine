#include "task/task_system.hpp"

namespace violet
{
task_system::task_system() noexcept : engine_system("task")
{
}

bool task_system::initialize(const dictionary& config)
{
    m_executor = std::make_unique<task_executor>();
    m_executor->run();

    m_task_graph.get_root().then(
        [this](float delta)
        {
            m_executor->execute_sync(m_frame_begin);
            m_executor->execute_sync(m_tick, delta);
            m_executor->execute_sync(m_frame_end);
        });

    return true;
}

void task_system::update(float delta)
{
    m_future = m_executor->execute(m_task_graph, delta);
}

void task_system::late_update(float delta)
{
    m_future.get();
}

void task_system::shutdown()
{
    m_executor->stop();
}
} // namespace violet
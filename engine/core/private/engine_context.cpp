#include "engine_context.hpp"

namespace violet
{
engine_context::engine_context()
    : m_exit(true)
{
    m_timer = std::make_unique<timer>();
    m_world = std::make_unique<world>();

    task_group& pre_update = m_task_graph.add_group().set_name("PreUpdate Group");
    task_group& update =
        m_task_graph.add_group().set_name("Update Group").add_dependency(pre_update);
    task_group& post_update =
        m_task_graph.add_group().set_name("Post Update Group").add_dependency(update);
}

engine_context::~engine_context() {}

void engine_context::set_system(std::size_t index, engine_system* system)
{
    if (index >= m_systems.size())
    {
        m_systems.resize(index + 1);
    }

    assert(m_systems[index] == nullptr);
    m_systems[index] = system;
}

engine_system* engine_context::get_system(std::size_t index)
{
    return m_systems[index];
}

void engine_context::tick()
{
    m_executor.execute_sync(m_task_graph);
    m_world->add_version();
}
} // namespace violet
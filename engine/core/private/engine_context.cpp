#include "engine_context.hpp"

namespace violet
{
engine_context::engine_context()
    : m_exit(true)
{
    m_timer = std::make_unique<timer>();
    m_world = std::make_unique<world>();

    auto& pre_update = m_task_graph.add_group().set_name("PreUpdate");
    auto& update = m_task_graph.add_group().set_name("Update").add_dependency(pre_update);
    auto& post_update = m_task_graph.add_group().set_name("PostUpdate").add_dependency(update);
}

engine_context::~engine_context() {}

void engine_context::set_system(std::size_t index, system* system)
{
    if (index >= m_systems.size())
    {
        m_systems.resize(index + 1);
    }

    assert(m_systems[index] == nullptr);
    m_systems[index] = system;
}
} // namespace violet
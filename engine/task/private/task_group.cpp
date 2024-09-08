#include "task/task_group.hpp"
#include "task/task_graph.hpp"
#include <cassert>

namespace violet
{
task_group::task_group(task_graph* graph)
    : m_graph(graph)
{
    m_begin = &graph->add_task();
    m_finish = &graph->add_task();

    m_finish->add_dependency(*m_begin);
}

task_group& task_group::set_group(task_group& group)
{
    assert(m_group == nullptr);

    m_group = &group;

    add_dependency(group.get_begin_task());
    group.get_finish_task().add_dependency(get_finish_task());

    return *this;
}

void task_group::add_dependency_impl(task& dependency)
{
    m_begin->add_dependency(dependency);
}

void task_group::add_dependency_impl(task_group& dependency)
{
    m_begin->add_dependency(dependency.get_finish_task());
}
} // namespace violet
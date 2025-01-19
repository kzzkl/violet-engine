#include "task/task.hpp"
#include "task/task_graph.hpp"
#include <cassert>

namespace violet
{
task::task(task_graph* graph) noexcept
    : m_graph(graph)
{
}

task::~task() {}

task& task::set_group(task_group& group)
{
    assert(m_group == nullptr);

    m_group = &group;

    add_dependency(group.get_begin_task());
    group.get_end_task().add_dependency(*this);
    return *this;
}

void task::add_dependency_impl(task& dependency)
{
    dependency.m_successors.push_back(this);
    m_dependencies.push_back(&dependency);

    m_graph->notify_task_change();
}

void task::add_dependency_impl(task_group& dependency)
{
    add_dependency(dependency.get_end_task());
}
} // namespace violet
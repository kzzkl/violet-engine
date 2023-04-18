#include "core/task/task.hpp"

namespace violet
{
struct engine_task_graph
{
    task_graph<> begin_frame;
    task_graph<float> tick;
    task_graph<> end_frame;
};
} // namespace violet
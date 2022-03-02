#include "application.hpp"
#include "timer.hpp"

using namespace ash::common;
using namespace ash::task;

namespace ash::core
{
application::application(const ash::common::dictionary& config) : context(config)
{
}

void application::run()
{
    std::chrono::time_point<steady_clock> frame_start;
    std::chrono::time_point<steady_clock> frame_end;

    std::size_t counter = 0;

    auto& task = get_task();

    auto root_task = task.schedule("root", []() {});

    task.schedule_before("begin", [&]() { frame_start = timer::now<steady_clock>(); });

    initialize_submodule();

    task.schedule_after("end", [&]() {
        frame_end = timer::now<steady_clock>();

        auto du = frame_end - frame_start;
        ++counter;
    });

    task.run(root_task);
}
} // namespace ash::core
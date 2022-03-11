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
    using namespace std::chrono;

    time_point<steady_clock> frame_start;
    time_point<steady_clock> frame_end;
    std::size_t frame_counter = 0;

    nanoseconds s(0);
    nanoseconds time_per_frame(1000000000 / 2400);

    auto& task = get_task();

    auto root_task = task.schedule("root", []() {});
    task.schedule_before("begin", [&]() { frame_start = timer::now<steady_clock>(); });

    initialize_submodule();

    task.schedule_after("end", [&]() {
        frame_end = timer::now<steady_clock>();

        nanoseconds delta = frame_end - frame_start;

        if (delta < time_per_frame)
            timer::busy_sleep(time_per_frame - delta);

        s += (timer::now<steady_clock>() - frame_start);

        ++frame_counter;
        if (s > seconds(1))
        {
            log::debug("FPS[{}] delta[{}]", frame_counter, delta.count());
            s = s.zero();
            frame_counter = 0;
        }
    });

    task.run(root_task);
}
} // namespace ash::core
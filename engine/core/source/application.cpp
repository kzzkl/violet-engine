#include "application.hpp"
#include "timer.hpp"

using namespace ash::task;

namespace ash::core
{
application::application(std::string_view config_path) : context(config_path)
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

    auto& task = get_submodule<task::task_manager>();
    task.run();

    auto root_task = task.find("root");

    while (true)
    {
        frame_start = timer::now<steady_clock>();

        task.execute(root_task);

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
    }
}
} // namespace ash::core
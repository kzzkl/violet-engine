#include "application.hpp"

using namespace ash::task;

namespace ash::core
{
application::application(std::string_view config_path) : context(config_path)
{
}

void application::run()
{
    using namespace std::chrono;

    std::size_t frame_counter = 0;

    nanoseconds s(0);
    nanoseconds time_per_frame(1000000000 / 240);

    auto& task = module<task::task_manager>();
    task.run();

    auto root_task = task.find("root");

    timer& time = module<timer>();
    while (!m_exit)
    {
        time.tick<timer::point::FRAME_START>();
        task.execute(root_task);
        time.tick<timer::point::FRAME_END>();

        nanoseconds delta = time.delta<timer::point::FRAME_START, timer::point::FRAME_END>();
        if (delta < time_per_frame)
            timer::busy_sleep(time_per_frame - delta);

        s += (timer::now<steady_clock>() - time.time_point<timer::point::FRAME_START>());

        ++frame_counter;
        if (s > seconds(1))
        {
            log::debug("FPS[{}] delta[{}]", frame_counter, delta.count());
            s = s.zero();
            frame_counter = 0;
        }
    }

    shutdown_submodule();
}

void application::exit()
{
    m_exit = true;
}
} // namespace ash::core
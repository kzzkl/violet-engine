#include "core/application.hpp"
#include "core/event.hpp"
#include "core/timer.hpp"
#include "ecs/world.hpp"
#include "task/task_manager.hpp"

using namespace ash::task;

namespace ash::core
{
application::application(std::string_view config_path)
{
    context::initialize(config_path);

    context::install<task::task_manager>();
    context::install<ecs::world>();
    context::install<event>();
    context::install<timer>();
}

void application::run()
{
    using namespace std::chrono;

    std::size_t frame_counter = 0;

    nanoseconds s(0);
    nanoseconds time_per_frame(1000000000 / 240);

    auto& task = system<task::task_manager>();
    task.run();

    auto root_task = task.find(task::TASK_ROOT);

    timer& time = system<timer>();

    time.tick<timer::point::FRAME_START>();
    time.tick<timer::point::FRAME_END>();
    while (!m_exit)
    {
        time.tick<timer::point::FRAME_START>();
        task.execute(root_task);
        time.tick<timer::point::FRAME_END>();

        nanoseconds delta = time.delta<timer::point::FRAME_START, timer::point::FRAME_END>();
        if (delta < time_per_frame)
            ; // timer::busy_sleep(time_per_frame - delta);

        s += (timer::now<steady_clock>() - time.time_point<timer::point::FRAME_START>());

        ++frame_counter;
        if (s > seconds(1))
        {
            // log::debug("FPS[{}] delta[{}]", frame_counter, delta.count());
            s = nanoseconds::zero();
            frame_counter = 0;
        }
    }

    context::shutdown();
}

void application::exit()
{
    m_exit = true;
}
} // namespace ash::core
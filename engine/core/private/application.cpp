#include "core/application.hpp"
#include "core/event.hpp"
#include "core/timer.hpp"
#include "ecs/world.hpp"
#include "task/task_manager.hpp"

using namespace violet::task;

namespace violet::core
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

    auto& task = system<task::task_manager>();
    task.run();

    auto root_task = task.find(task::TASK_ROOT);

    timer& time = system<timer>();

    time.tick<timer::point::FRAME_START>();
    time.tick<timer::point::FRAME_END>();
    while (!m_exit)
    {
        time.tick<timer::point::FRAME_START>();
        context::begin_frame();
        task.execute(root_task);
        context::end_frame();
        time.tick<timer::point::FRAME_END>();

        /*
        static constexpr nanoseconds time_per_frame(1000000000 / 240);
        nanoseconds delta = time.delta<timer::point::FRAME_START, timer::point::FRAME_END>();
        if (delta < time_per_frame)
            timer::busy_sleep(time_per_frame - delta);
        */
    }

    context::shutdown();
}

void application::exit()
{
    m_exit = true;
}
} // namespace violet::core
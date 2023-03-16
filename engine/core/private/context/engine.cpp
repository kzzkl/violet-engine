#include "core/context/engine.hpp"
#include "core/node/world.hpp"
#include "core/task/task_manager.hpp"
#include <filesystem>
#include <fstream>

namespace violet::core
{
template <std::uint32_t FPS>
class frame_rater
{
public:
    frame_rater() : m_time_between_frames(1), m_time_point(std::chrono::steady_clock::now()) {}

    void sleep()
    {
        m_time_point += m_time_between_frames;
        std::this_thread::sleep_until(m_time_point);
    }

private:
    using sleep_duration = std::chrono::duration<double, std::ratio<1, FPS>>;
    using sleep_time_point = std::chrono::time_point<std::chrono::steady_clock, sleep_duration>;

    sleep_duration m_time_between_frames;
    sleep_time_point m_time_point;
};

engine::engine() : m_exit(true)
{
}

engine& engine::instance()
{
    static engine instance;
    return instance;
}

void engine::initialize(std::string_view config_path)
{
    auto& singleton = instance();

    for (auto iter : std::filesystem::directory_iterator("engine/config"))
    {
        if (iter.is_regular_file() && iter.path().extension() == ".json")
        {
            std::ifstream fin(iter.path());
            if (!fin.is_open())
                continue;

            dictionary config;
            fin >> config;

            for (auto& [key, value] : config.items())
                singleton.m_config[key].update(value, true);
        }
    }

    if (config_path != "")
    {
        for (auto iter : std::filesystem::directory_iterator(config_path))
        {
            if (iter.is_regular_file() && iter.path().extension() == ".json")
            {
                std::ifstream fin(iter.path());
                if (!fin.is_open())
                    continue;

                dictionary config;
                fin >> config;

                for (auto& [key, value] : config.items())
                    singleton.m_config[key].update(value, true);
            }
        }
    }

    singleton.m_event = std::make_unique<event>();
    singleton.m_timer = std::make_unique<timer>();
    singleton.m_world = std::make_unique<world>();

    std::size_t thread_count = singleton.m_config["engine"]["task_thread_count"];
    singleton.m_task_manager = std::make_unique<task_manager>(thread_count);
}

void engine::run()
{
    using namespace std::chrono;
    auto& singleton = instance();

    if (!singleton.m_exit)
        return;
    else
        singleton.m_exit = false;

    singleton.m_task_manager->run();
    auto root_task = singleton.m_task_manager->find(TASK_ROOT);

    frame_rater<60> frame_rater;
    timer& time = get_timer();

    time.tick<timer::point::FRAME_START>();
    time.tick<timer::point::FRAME_END>();
    while (!singleton.m_exit)
    {
        time.tick<timer::point::FRAME_START>();
        engine::begin_frame();
        singleton.m_task_manager->execute(root_task);
        engine::end_frame();
        time.tick<timer::point::FRAME_END>();

        frame_rater.sleep();
    }

    // shutdown
    for (auto iter = singleton.m_installation_sequence.rbegin();
         iter != singleton.m_installation_sequence.rend();
         ++iter)
    {
        log::info("Module shutdown: {}.", singleton.m_modules[*iter]->name());
        singleton.m_modules[*iter]->shutdown();
        singleton.m_modules[*iter] = nullptr;
    }
}

void engine::exit()
{
    instance().m_exit = true;
}

void engine::begin_frame()
{
    auto& singleton = instance();

    for (auto& module : singleton.m_modules)
        module->on_begin_frame();
}

void engine::end_frame()
{
    auto& singleton = instance();

    for (auto& module : singleton.m_modules)
        module->on_end_frame();
}
} // namespace violet::core
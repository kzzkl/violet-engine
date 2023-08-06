#include "core/engine.hpp"
#include "core/node/world.hpp"
#include <filesystem>
#include <fstream>

namespace violet
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

    singleton.m_timer = std::make_unique<timer>();
    singleton.m_world = std::make_unique<world>();
    singleton.m_task_executor = std::make_unique<task_executor>();
}

void engine::run()
{
    instance().main_loop();
}

void engine::exit()
{
    instance().m_exit = true;
}

void engine::main_loop()
{
    if (!m_exit)
        return;
    else
        m_exit = false;

    m_task_executor->run();

    frame_rater<60> frame_rater;
    timer& time = get_timer();

    time.tick(timer::point::FRAME_START);
    time.tick(timer::point::FRAME_END);

    while (!m_exit)
    {
        time.tick(timer::point::FRAME_START);

        m_task_executor->execute_sync(m_frame_begin);
        m_task_executor->execute_sync(m_tick, time.get_frame_delta());
        m_task_executor->execute_sync(m_frame_end);

        time.tick(timer::point::FRAME_END);

        frame_rater.sleep();
    }
    m_task_executor->stop();

    // shutdown
    for (auto iter = m_install_sequence.rbegin(); iter != m_install_sequence.rend(); ++iter)
    {
        log::info("System shutdown: {}.", m_systems[*iter]->get_name());
        m_systems[*iter]->shutdown();
        m_systems[*iter] = nullptr;
    }
}

bool engine::has_system(std::string_view name)
{
    auto& singleton = instance();

    for (auto& system : singleton.m_systems)
    {
        if (system->get_name() == name)
            return true;
    }

    return false;
}

void engine::uninstall(std::size_t index)
{
    assert(m_systems.size() > index);

    if (m_systems[index] != nullptr)
    {
        m_systems[index]->shutdown();
        m_systems[index] = nullptr;

        for (auto iter = m_install_sequence.begin(); iter != m_install_sequence.end(); ++iter)
        {
            if (*iter == index)
            {
                m_install_sequence.erase(iter);
                break;
            }
        }
        log::info("System uninstalled successfully: {}.", m_systems[index]->get_name());
    }
    else
    {
        log::warn("The system is not installed.");
    }
}
} // namespace violet
#include "core/engine.hpp"
#include "common/log.hpp"
#include "engine_context.hpp"
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

engine::~engine()
{
}

void engine::initialize(std::string_view config_path)
{
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
                m_config[key].update(value, true);
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
                    m_config[key].update(value, true);
            }
        }
    }

    m_context = std::make_unique<engine_context>();
}

void engine::run()
{
    if (!m_exit)
        return;
    else
        m_exit = false;

    task_executor& executor = m_context->get_task_executor();
    executor.run();

    frame_rater<30> frame_rater;
    timer& time = m_context->get_timer();
    time.tick(timer::point::FRAME_START);
    time.tick(timer::point::FRAME_END);

    while (!m_exit)
    {
        time.tick(timer::point::FRAME_START);

        executor.execute_sync(m_context->get_frame_begin_task());
        executor.execute_sync(m_context->get_tick_task(), time.get_frame_delta());
        executor.execute_sync(m_context->get_frame_end_task());

        time.tick(timer::point::FRAME_END);

        // frame_rater.sleep();
    }
    executor.stop();

    // shutdown
    for (auto iter = m_systems.rbegin(); iter != m_systems.rend(); ++iter)
    {
        log::info("System shutdown: {}.", (*iter)->get_name());
        (*iter)->shutdown();
        (*iter) = nullptr;
    }
}

void engine::exit()
{
    m_exit = true;
}

void engine::install(std::size_t index, std::unique_ptr<engine_system>&& system)
{
    system->m_context = m_context.get();
    if (!system->initialize(m_config[system->get_name().data()]))
        throw std::runtime_error(system->get_name() + " initialize failed");

    m_context->set_system(index, system.get());
    log::info("System installed successfully: {}.", system->get_name());
    m_systems.push_back(std::move(system));
}

void engine::uninstall(std::size_t index)
{
    engine_system* system = m_context->get_system(index);
    if (system)
    {
        system->shutdown();
        m_context->set_system(index, nullptr);

        for (auto iter = m_systems.begin(); iter != m_systems.end(); ++iter)
        {
            if ((*iter).get() == system)
            {
                m_systems.erase(iter);
                break;
            }
        }

        log::info("System uninstalled successfully: {}.", system->get_name());
    }
    else
    {
        log::warn("The system is not installed.");
    }
}

engine_system* engine::get_system(std::size_t index)
{
    return m_context->get_system(index);
}
} // namespace violet
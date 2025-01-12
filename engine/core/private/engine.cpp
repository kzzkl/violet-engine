#include "core/engine.hpp"
#include "common/log.hpp"
#include "common/utility.hpp"
#include "engine_context.hpp"
#include "task/task_graph_printer.hpp"
#include <fstream>

namespace violet
{
template <std::uint32_t FPS>
class frame_rater
{
public:
    frame_rater()
        : m_time_between_frames(1),
          m_time_point(std::chrono::steady_clock::now())
    {
    }

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

system::system(std::string_view name) noexcept
    : m_name(name),
      m_context(nullptr)
{
}

system* system::get_system(std::size_t index)
{
    return m_context->get_system(index);
}

timer& system::get_timer()
{
    return m_context->get_timer();
}

world& system::get_world()
{
    return m_context->get_world();
}

task_graph& system::get_task_graph() noexcept
{
    return m_context->get_task_graph();
}

task_executor& system::get_task_executor() noexcept
{
    return m_context->get_task_executor();
}

application::application(std::string_view config_path)
{
    std::vector<std::wstring> config_files;
    config_files.emplace_back(L"assets/config/default.json");
    config_files.emplace_back(string_to_wstring(config_path));

    for (const auto& file : config_files)
    {
        std::ifstream fin(file);
        if (!fin.is_open())
        {
            continue;
        }

        dictionary json;
        fin >> json;

        for (const auto& [key, value] : json.items())
        {
            m_config[key].update(value, true);
        }
        fin.close();
    }

    m_context = std::make_unique<engine_context>();
}

application::~application() = default;

void application::run()
{
    if (!m_exit)
    {
        return;
    }

    m_exit = false;

    frame_rater<30> frame_rater;
    timer& time = m_context->get_timer();
    time.tick(timer::point::FRAME_START);
    time.tick(timer::point::FRAME_END);

    auto& executor = m_context->get_task_executor();
    auto& world = m_context->get_world();

    m_context->get_task_graph().reset();
    task_graph_printer::print(m_context->get_task_graph());

    executor.run(m_config["engine"]["task_thread_count"]);

    while (!m_exit)
    {
        time.tick(timer::point::FRAME_START);

        executor.execute_sync(m_context->get_task_graph());
        world.add_version();

        time.tick(timer::point::FRAME_END);

        // frame_rater.sleep();
    }

    executor.stop();
    world.clear();

    // shutdown
    std::for_each(
        m_systems.rbegin(),
        m_systems.rend(),
        [](auto& system)
        {
            log::info("System shutdown: {}.", system->get_name());
            system->shutdown();
            system = nullptr;
        });
}

void application::exit()
{
    m_exit = true;
}

void application::install(std::size_t index, std::unique_ptr<system>&& system)
{
    system->m_context = m_context.get();
    system->install(*this);
    if (!system->initialize(m_config[system->get_name().data()]))
    {
        throw std::runtime_error(system->get_name() + " initialize failed");
    }

    m_context->set_system(index, system.get());
    log::info("System installed successfully: {}.", system->get_name());
    m_systems.push_back(std::move(system));
}

void application::uninstall(std::size_t index)
{
    auto* system = m_context->get_system(index);
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

bool application::has_system(std::size_t index) const noexcept
{
    return m_context->has_system(index);
}
} // namespace violet
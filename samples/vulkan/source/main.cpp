#include "application.hpp"
#include "context.hpp"
#include "graphics_interface.hpp"
#include "plugin.hpp"
#include "window.hpp"

namespace ash::sample
{
class vulkan_plugin : public core::plugin
{
public:
    bool initialize(const graphics::context_config& config)
    {
        m_context->initialize(config);
        //m_context->deinitialize();

        return true;
    }

    graphics::renderer* renderer()
    {
        return m_context->renderer();
    }

protected:
    virtual bool do_load() override
    {
        graphics::make_context make =
            static_cast<graphics::make_context>(find_symbol("make_context"));
        if (make == nullptr)
        {
            log::error("Symbol not found in plugin: make_context.");
            return false;
        }

        m_context.reset(make());
        return true;
    }

private:
    std::unique_ptr<graphics::context> m_context;
};

class test_system : public core::system_base
{
public:
    test_system() : core::system_base("test") {}

    virtual bool initialize(const dictionary& config) override
    {
        initialize_vulkan();
        initialize_task();
        return true;
    }

private:
    void initialize_task()
    {
        auto& task = system<task::task_manager>();
        auto& window = system<window::window>();

        auto window_task = task.schedule(
            "update",
            [&, this]() { window.tick(); },
            task::task_type::MAIN_THREAD);
        window_task->add_dependency(*task.find("root"));

        auto render_task = task.schedule("render", [&, this]() {
            m_vulkan_plugin.renderer()->begin_frame();
            m_vulkan_plugin.renderer()->end_frame();
        });
        render_task->add_dependency(*window_task);
    }

    void initialize_vulkan()
    {
        m_vulkan_plugin.load("ash-graphics-vulkan.dll");

        auto rect = system<window::window>().rect();

        graphics::context_config config;
        config.width = rect.width;
        config.height = rect.height;
        config.window_handle = system<window::window>().handle();
        m_vulkan_plugin.initialize(config);
    }

    vulkan_plugin m_vulkan_plugin;
};

class vulkan_app
{
public:
    void initialize()
    {
        m_app.install<window::window>();
        m_app.install<test_system>();
    }

    void run() { m_app.run(); }

private:
    core::application m_app;
};
} // namespace ash::sample

int main()
{
    ash::sample::vulkan_app app;
    app.initialize();
    app.run();

    return 0;
}
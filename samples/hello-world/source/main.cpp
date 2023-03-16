#include "core/context/engine.hpp"
#include "core/node/node.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"
#include <filesystem>
#include <fstream>
#include <thread>

using namespace violet::core;

namespace violet::sample
{
class hello_world : public core::engine_module
{
public:
    hello_world() : core::engine_module("hello_world") {}

    virtual bool initialize(const dictionary& config)
    {
        violet::log::info(config["text"]);

        auto& event = engine::get_event();
        event.subscribe<window::event_window_destroy>("hello_world", []() {
            violet::log::info("close window");
            engine::exit();
        });

        m_test_object = std::make_unique<core::node>("test");

        return true;
    }

    virtual void shutdown() {}

private:
    std::unique_ptr<core::node> m_test_object;
};
} // namespace violet::sample

int main()
{
    engine::initialize("");
    engine::install<violet::window::window>();
    engine::install<violet::sample::hello_world>();
    engine::run();

    return 0;
}
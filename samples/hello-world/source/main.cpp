#include "core/context/engine.hpp"
#include "core/node/node.hpp"
#include "graphics/graphics.hpp"
#include "window/window.hpp"
#include "window/window_task.hpp"
#include <filesystem>
#include <fstream>
#include <thread>

namespace violet::sample
{
class hello_world : public engine_module
{
public:
    hello_world() : engine_module("hello_world") {}

    virtual bool initialize(const dictionary& config)
    {
        log::info(config["text"]);

        auto& window_module = engine::get_module<window>();
        window_module.get_task_graph().window_destroy.add_task("exit", []() {
            log::info("close window");
            engine::exit();
        });

        m_test_object = std::make_unique<node>("test");

        return true;
    }

    virtual void shutdown() {}

private:
    std::unique_ptr<node> m_test_object;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine::initialize("");
    engine::install<window>();
    engine::install<graphics>();
    engine::install<sample::hello_world>();
    engine::run();

    return 0;
}
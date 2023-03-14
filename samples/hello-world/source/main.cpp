#include "core/context/engine.hpp"
#include "core/node/node.hpp"
#include "window/window.hpp"
#include <filesystem>
#include <fstream>

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

        auto& world = engine::get_world();

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
#include "common/log.hpp"
#include "components/mesh.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_module.hpp"
#include "scene/scene_module.hpp"
#include "window/window_module.hpp"
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
        initialize_render_graph();

        return true;
    }

    virtual void shutdown() {}

private:
    void initialize_render_graph()
    {
    }

    void initialize_object()
    {
    }

    std::unique_ptr<material> m_material;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine engine;
    engine.initialize("hello-world/config");
    engine.install<window_module>();
    engine.install<scene_module>();
    engine.install<graphics_module>();
    engine.install<sample::hello_world>();

    engine.get_module<window_module>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();

    return 0;
}
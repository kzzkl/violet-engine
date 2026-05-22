#include "core/engine.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "task/task_executor.hpp"
#include "task/task_graph.hpp"
#include "ecs/world.hpp"
#include <catch2/catch_all.hpp>

namespace violet::test
{
world*            g_world            = nullptr;
transform_system* g_transform_system = nullptr;
task_graph*       g_task_graph       = nullptr;
task_executor*    g_task_executor    = nullptr;

class scene_test_helper : public system
{
public:
    scene_test_helper() : system("scene-test-helper") {}

    bool initialize(const dictionary& config) override
    {
        g_world            = &get_world();
        g_transform_system = &get_system<transform_system>();
        g_task_graph       = &get_task_graph();
        g_task_executor    = &get_task_executor();
        return true;
    }
};
} // namespace violet::test

int main(int argc, char* argv[])
{
    static violet::application app;
    app.install<violet::scene_system>();
    app.install<violet::test::scene_test_helper>();

    return Catch::Session().run(argc, argv);
}
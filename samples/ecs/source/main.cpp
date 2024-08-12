#include "common/log.hpp"
#include "components/hierarchy.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"

namespace violet::sample
{
class ecs_sample : public engine_system
{
public:
    ecs_sample()
        : engine_system("ECS Sample")
    {
    }

    bool initialize(const dictionary& config) override
    {
        auto& world = get_world();

        m_parent = world.create();
        world.add_component<transform, transform_local, transform_world>(m_parent);

        m_child = world.create();
        world.add_component<transform, transform_local, transform_world>(m_child);

        auto& t = world.get_component<transform>(m_parent);
        t.position = {1.0f, 2.0f, 3.0f};

        get_taskflow()
            .add_task([this]() { test(); })
            .set_name("ECS Test")
            .add_successor("Transform Local Update");

        get_taskflow()
            .add_task([this]() { update(); })
            .set_name("ECS Sample")
            .add_predecessor("Transform Local Update");

        return true;
    }

private:
    void test()
    {
        log::info("ECS Sample: Test");

        auto& local = get_world().get_component<transform_local>(m_parent);
        int a = 10;
    }

    void update()
    {
        log::info("ECS Sample: update");

        auto& local = get_world().get_component<transform_local>(m_parent);
        auto& world = get_world().get_component<transform_world>(m_parent);
        int a = 10;
    }

    entity m_parent;
    entity m_child;
};
} // namespace violet::sample

int main()
{
    violet::engine::initialize("");
    violet::engine::install<violet::transform_system>();
    violet::engine::install<violet::sample::ecs_sample>();

    violet::engine::run();

    return 0;
}
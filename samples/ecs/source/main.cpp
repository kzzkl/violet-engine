#include "common/log.hpp"
#include "components/hierarchy.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "scene/hierarchy_system.hpp"
#include "scene/scene_system.hpp"
#include "scene/transform_system.hpp"
#include "window/window_system.hpp"

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

        world.register_component<std::string>();

        m_parent = world.create();
        world.add_component<std::string, transform, transform_local, transform_world>(m_parent);
        world.get_component<std::string>(m_parent) = "Parent";
        world.get_component<transform>(m_parent).position = {1.0f, 2.0f, 3.0f};

        m_child = world.create();
        world.add_component<
            std::string,
            hierarchy_parent,
            transform,
            transform_local,
            transform_world>(m_child);
        world.get_component<std::string>(m_child) = "Child";
        world.get_component<hierarchy_parent>(m_child).parent = m_parent;

        task_graph& task_graph = get_task_graph();
        task& update_transform = task_graph.get_task("Update Transform");

        task_graph.add_task()
            .set_name("ECS Test")
            .add_dependency(update_transform)
            .set_execute(
                [this]()
                {
                    update();
                    m_system_version = get_world().get_version();
                });

        get_system<window_system>().on_destroy().add_task().set_execute(
            []()
            {
                log::info("Close window");
                engine::exit();
            });

        return true;
    }

private:
    void update()
    {
        world& world = get_world();

        auto view = world.get_view().read<std::string>().read<transform_world>();
        view.each(
            [](const std::string& name, const transform_world& world)
            {
                log::info(
                    "Entity: {}, World Position: {}, {}, {}",
                    name,
                    world.matrix[3][0],
                    world.matrix[3][1],
                    world.matrix[3][2]);
            },
            [this](auto& view)
            {
                return true || view.is_updated<transform_world>(m_system_version);
            });
    }

    entity m_parent;
    entity m_child;

    std::uint32_t m_system_version{0};
};
} // namespace violet::sample

int main()
{
    violet::engine::initialize("");
    violet::engine::install<violet::hierarchy_system>();
    violet::engine::install<violet::transform_system>();
    violet::engine::install<violet::window_system>();
    violet::engine::install<violet::sample::ecs_sample>();

    violet::engine::run();

    return 0;
}
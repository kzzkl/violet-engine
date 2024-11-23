#include "scene/hierarchy_system.hpp"
#include "components/hierarchy.hpp"

namespace violet
{
hierarchy_system::hierarchy_system()
    : engine_system("hierarchy")
{
}

bool hierarchy_system::initialize(const dictionary& config)
{
    get_world().register_component<hierarchy_parent>();
    get_world().register_component<hierarchy_previous_parent>();
    get_world().register_component<hierarchy_child>();

    task_graph& task_graph = get_task_graph();
    task_group& post_update_group = task_graph.get_group("Post Update Group");

    task_graph.add_task()
        .set_name("Update Hierarchy")
        .set_group(post_update_group)
        .set_options(TASK_OPTION_MAIN_THREAD)
        .set_execute(
            [this]()
            {
                process_add_parent();
                process_set_parent();
                process_remove_parent();
                m_system_version = get_world().get_version();
            });

    return true;
}

void hierarchy_system::process_add_parent()
{
    auto& world = get_world();

    std::vector<std::pair<entity, entity>> add_parent_entities;
    std::vector<std::pair<entity, entity>> add_child_entities;

    world.get_view()
        .read<entity>()
        .read<hierarchy_parent>()
        .without<hierarchy_previous_parent>()
        .each(
            [&world, &add_parent_entities, &add_child_entities](
                const entity& e, const hierarchy_parent& parent)
            {
                add_parent_entities.push_back({e, parent.parent});

                if (!world.has_component<hierarchy_child>(parent.parent))
                {
                    add_child_entities.push_back({parent.parent, e});
                }
                else
                {
                    world.get_component<hierarchy_child>(parent.parent).children.push_back(e);
                }
            });

    for (auto& [e, parent] : add_parent_entities)
    {
        world.add_component<hierarchy_previous_parent>(e);
        world.get_component<hierarchy_previous_parent>(e).parent = parent;
    }

    for (auto& [e, child] : add_child_entities)
    {
        world.add_component<hierarchy_child>(e);
        world.get_component<hierarchy_child>(e).children.push_back(child);
    }
}

void hierarchy_system::process_set_parent()
{
    auto& world = get_world();

    std::vector<entity> remove_child_entities;
    std::vector<entity> remove_parent_entities;
    std::vector<std::pair<entity, entity>> add_child_entities;

    world.get_view().read<entity>().read<hierarchy_parent>().read<hierarchy_previous_parent>().each(
        [&world, &remove_child_entities, &remove_parent_entities, &add_child_entities](
            const entity& e,
            const hierarchy_parent& parent,
            const hierarchy_previous_parent& previous_parent)
        {
            if (parent.parent == previous_parent.parent)
            {
                return;
            }

            hierarchy_child& previous_parent_children =
                world.get_component<hierarchy_child>(previous_parent.parent);

            auto iter = std::find(
                previous_parent_children.children.begin(),
                previous_parent_children.children.end(),
                e);
            std::swap(*iter, previous_parent_children.children.back());
            previous_parent_children.children.pop_back();

            if (previous_parent_children.children.empty())
            {
                remove_child_entities.push_back(previous_parent.parent);
            }

            if (parent.parent == entity_null)
            {
                remove_parent_entities.push_back(e);
            }
            else
            {
                if (!world.has_component<hierarchy_child>(parent.parent))
                {
                    add_child_entities.push_back({parent.parent, e});
                }
                else
                {
                    world.get_component<hierarchy_child>(parent.parent).children.push_back(e);
                }
            }
        },
        [this](auto& view)
        {
            return view.is_updated<hierarchy_parent>(m_system_version);
        });

    for (auto& e : remove_child_entities)
    {
        world.remove_component<hierarchy_child>(e);
    }

    for (auto& e : remove_parent_entities)
    {
        world.remove_component<hierarchy_parent>(e);
    }

    for (auto& [e, child] : add_child_entities)
    {
        world.add_component<hierarchy_child>(e);
        world.get_component<hierarchy_child>(e).children.push_back(child);
    }
}

void hierarchy_system::process_remove_parent()
{
    auto& world = get_world();

    std::vector<entity> remove_child_entities;
    std::vector<entity> remove_previous_parent_entities;

    world.get_view()
        .read<entity>()
        .read<hierarchy_previous_parent>()
        .without<hierarchy_parent>()
        .each(
            [&world, &remove_child_entities, &remove_previous_parent_entities](
                const entity& e, const hierarchy_previous_parent& previous_parent)
            {
                hierarchy_child& previous_parent_children =
                    world.get_component<hierarchy_child>(previous_parent.parent);

                auto iter = std::find(
                    previous_parent_children.children.begin(),
                    previous_parent_children.children.end(),
                    e);
                std::swap(*iter, previous_parent_children.children.back());
                previous_parent_children.children.pop_back();

                if (previous_parent_children.children.empty())
                {
                    remove_child_entities.push_back(previous_parent.parent);
                }

                remove_previous_parent_entities.push_back(e);
            });

    for (auto& e : remove_child_entities)
    {
        world.remove_component<hierarchy_child>(e);
    }

    for (auto& e : remove_previous_parent_entities)
    {
        world.remove_component<hierarchy_previous_parent>(e);
    }
}
} // namespace violet
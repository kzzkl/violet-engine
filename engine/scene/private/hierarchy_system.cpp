#include "scene/hierarchy_system.hpp"
#include "components/hierarchy.hpp"

namespace violet
{
hierarchy_system::hierarchy_system()
    : engine_system("Hierarchy")
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
        .set_execute(
            [this]()
            {
                add_previous_parent();
                add_child();
                update_child();
            });

    return true;
}

void hierarchy_system::add_previous_parent()
{
    world& world = get_world();

    auto view = world.get_view()
                    .read<entity>()
                    .read<hierarchy_parent>()
                    .without<hierarchy_previous_parent>();

    std::vector<entity> entities;
    view.each(
        [&entities](const entity& e, const hierarchy_parent& parent)
        {
            entities.push_back(e);
        });

    for (auto& e : entities)
    {
        world.add_component<hierarchy_previous_parent>(e);
    }
}

void hierarchy_system::add_child()
{
    world& world = get_world();

    std::vector<entity> missing_child_entities;
    world.get_view().read<hierarchy_parent>().read<hierarchy_previous_parent>().each(
        [&missing_child_entities,
         &world](const hierarchy_parent& parent, const hierarchy_previous_parent& previous_parent)
        {
            if (parent.parent != previous_parent.parent)
            {
                if (!world.has_component<hierarchy_child>(parent.parent))
                {
                    missing_child_entities.push_back(parent.parent);
                }
            }
        },
        [this](auto& view)
        {
            return view.is_updated<hierarchy_parent>(m_system_version);
        });

    for (auto& e : missing_child_entities)
    {
        world.add_component<hierarchy_child>(e);
    }
}

void hierarchy_system::update_child()
{
    world& world = get_world();

    std::vector<entity> remove_child_entities;

    world.get_view()
        .read<entity>()
        .read<hierarchy_parent>()
        .write<hierarchy_previous_parent>()
        .each(
            [this, &remove_child_entities, &world](
                const entity& e,
                const hierarchy_parent& parent,
                hierarchy_previous_parent& previous_parent)
            {
                if (parent.parent == previous_parent.parent)
                {
                    return;
                }

                if (previous_parent.parent != entity_null)
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
                }

                if (parent.parent != entity_null)
                {
                    hierarchy_child& new_parent_children =
                        world.get_component<hierarchy_child>(parent.parent);
                    new_parent_children.children.push_back(e);
                }

                previous_parent.parent = parent.parent;
            },
            [this](auto& view)
            {
                return view.is_updated<hierarchy_parent>(m_system_version);
            });

    for (auto& e : remove_child_entities)
    {
        world.remove_component<hierarchy_child>(e);
    }
}
} // namespace violet
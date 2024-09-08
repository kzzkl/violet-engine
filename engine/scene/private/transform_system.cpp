#include "scene/transform_system.hpp"
#include "components/hierarchy.hpp"
#include "components/transform.hpp"

namespace violet
{
transform_system::transform_system()
    : engine_system("Transform")
{
}

bool transform_system::initialize(const dictionary& config)
{
    get_world().register_component<transform>();
    get_world().register_component<transform_local>();
    get_world().register_component<transform_world>();

    task_graph& task_graph = get_task_graph();
    task_group& post_update_group = task_graph.get_group("Post Update Group");
    task& update_hierarchy = task_graph.get_task("Update Hierarchy");

    task_graph.add_task()
        .set_name("Update Transform")
        .set_group(post_update_group)
        .add_dependency(update_hierarchy)
        .set_execute(
            [this]()
            {
                update_local();
                update_world();
                m_system_version = get_world().get_version();
            });

    return true;
}

void transform_system::update_local()
{
    auto view = get_world().get_view().read<transform>().write<transform_local>();

    view.each(
        [](const transform& transform, transform_local& local)
        {
            matrix4 local_matrix = matrix::affine_transform(
                math::load(transform.scale),
                math::load(transform.rotation),
                math::load(transform.position));
            math::store(local_matrix, local.matrix);
        },
        [this](auto& view)
        {
            return view.is_updated<transform>(m_system_version);
        });
}

void transform_system::update_world()
{
    auto& world = get_world();

    world.get_view()
        .read<transform_local>()
        .write<transform_world>()
        .without<hierarchy_parent>()
        .each(
            [](const transform_local& local, transform_world& world)
            {
                world.matrix = local.matrix;
            },
            [this](auto& view)
            {
                return view.is_updated<transform_local>(m_system_version);
            });

    std::vector<entity> root_entities;

    world.get_view()
        .read<entity>()
        .read<transform_world>()
        .read<hierarchy_child>()
        .without<hierarchy_parent>()
        .each(
            [&root_entities](
                const entity& e, const transform_world& world, const hierarchy_child& child)
            {
                root_entities.push_back(e);
            });

    for (auto& root : root_entities)
    {
        if (world.has_component<hierarchy_child>(root))
        {
            float4x4 root_matrix = world.get_component<const transform_world>(root).matrix;
            for (auto& child : world.get_component<const hierarchy_child>(root).children)
            {
                update_world_recursive(child, root_matrix, false);
            }
        }
    }
}

void transform_system::update_world_recursive(
    entity e, const float4x4& parent_world, bool need_update)
{
    auto& world = get_world();

    need_update = need_update || world.is_updated<transform_local>(e, m_system_version) ||
                  world.is_updated<hierarchy_parent>(e, m_system_version);

    if (need_update)
    {
        matrix4 local_matrix = math::load(world.get_component<const transform_local>(e).matrix);
        matrix4 parent_matrix = math::load(parent_world);

        auto& world_transform = world.get_component<transform_world>(e);
        math::store(matrix::mul(local_matrix, parent_matrix), world_transform.matrix);

        if (world.has_component<hierarchy_child>(e))
        {
            for (auto& child : world.get_component<const hierarchy_child>(e).children)
            {
                update_world_recursive(child, world_transform.matrix, need_update);
            }
        }
    }
    else
    {
        if (world.has_component<hierarchy_child>(e))
        {
            auto& world_transform = world.get_component<const transform_world>(e);
            for (auto& child : world.get_component<const hierarchy_child>(e).children)
            {
                update_world_recursive(child, world_transform.matrix, need_update);
            }
        }
    }
}
} // namespace violet
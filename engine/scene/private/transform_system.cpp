#include "scene/transform_system.hpp"
#include "components/hierarchy_component.hpp"
#include "components/transform_component.hpp"

namespace violet
{
transform_system::transform_system()
    : engine_system("transform")
{
}

bool transform_system::initialize(const dictionary& config)
{
    get_world().register_component<transform_component>();
    get_world().register_component<transform_local_component>();
    get_world().register_component<transform_world_component>();

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
    auto view =
        get_world().get_view().read<transform_component>().write<transform_local_component>();

    view.each(
        [](const transform_component& transform, transform_local_component& local)
        {
            mat4f_simd local_matrix = matrix::affine_transform(
                math::load(transform.scale),
                math::load(transform.rotation),
                math::load(transform.position));
            math::store(local_matrix, local.matrix);
        },
        [this](auto& view)
        {
            return view.is_updated<transform_component>(m_system_version);
        });
}

void transform_system::update_world()
{
    auto& world = get_world();

    world.get_view()
        .read<transform_local_component>()
        .write<transform_world_component>()
        .without<parent_component>()
        .each(
            [](const transform_local_component& local, transform_world_component& world)
            {
                world.matrix = local.matrix;
            },
            [this](auto& view)
            {
                return view.is_updated<transform_local_component>(m_system_version);
            });

    std::vector<entity> root_entities;

    world.get_view()
        .read<entity>()
        .read<transform_world_component>()
        .read<child_component>()
        .without<parent_component>()
        .each(
            [&root_entities](
                const entity& e,
                const transform_world_component& world,
                const child_component& child)
            {
                root_entities.push_back(e);
            });

    for (auto& root : root_entities)
    {
        if (world.has_component<child_component>(root))
        {
            mat4f root_matrix = world.get_component<const transform_world_component>(root).matrix;
            for (auto& child : world.get_component<const child_component>(root).children)
            {
                update_world_recursive(child, root_matrix, false);
            }
        }
    }
}

void transform_system::update_world_recursive(entity e, const mat4f& parent_world, bool need_update)
{
    auto& world = get_world();

    need_update = need_update || world.is_updated<transform_local_component>(e, m_system_version) ||
                  world.is_updated<parent_component>(e, m_system_version);

    if (need_update)
    {
        mat4f_simd local_matrix =
            math::load(world.get_component<const transform_local_component>(e).matrix);
        mat4f_simd parent_matrix = math::load(parent_world);

        auto& world_transform = world.get_component<transform_world_component>(e);
        math::store(matrix::mul(local_matrix, parent_matrix), world_transform.matrix);

        if (world.has_component<child_component>(e))
        {
            for (auto& child : world.get_component<const child_component>(e).children)
            {
                update_world_recursive(child, world_transform.matrix, need_update);
            }
        }
    }
    else
    {
        if (world.has_component<child_component>(e))
        {
            auto& world_transform = world.get_component<const transform_world_component>(e);
            for (auto& child : world.get_component<const child_component>(e).children)
            {
                update_world_recursive(child, world_transform.matrix, need_update);
            }
        }
    }
}
} // namespace violet
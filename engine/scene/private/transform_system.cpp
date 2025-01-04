#include "scene/transform_system.hpp"
#include "components/hierarchy_component.hpp"
#include "components/transform_component.hpp"
#include "math/matrix.hpp"
#include "scene/hierarchy_system.hpp"

namespace violet
{
transform_system::transform_system()
    : system("transform")
{
}

void transform_system::install(application& app)
{
    app.install<hierarchy_system>();
}

bool transform_system::initialize(const dictionary& config)
{
    get_world().register_component<transform_component>();
    get_world().register_component<transform_local_component>();
    get_world().register_component<transform_world_component>();

    task_graph& task_graph = get_task_graph();
    task_group& transform_group = task_graph.get_group("Transform");
    task& update_hierarchy_task = task_graph.get_task("Update Hierarchy");

    task_graph.add_task()
        .set_name("Update Transform")
        .set_group(transform_group)
        .add_dependency(update_hierarchy_task)
        .set_execute(
            [this]()
            {
                update_local();
                update_world();
                m_system_version = get_world().get_version();
            });

    return true;
}

void transform_system::update_transform()
{
    update_local(true);
    update_world(true);
}

mat4f transform_system::get_local_matrix(entity e)
{
    auto& world = get_world();

    if (world.get_component<const transform_component>(e).is_local_dirty())
    {
        auto& transform = world.get_component<transform_component>(e);
        auto& local_transform = world.get_component<transform_local_component>(e);

        mat4f_simd local_matrix = matrix::affine_transform(
            math::load(transform.get_scale()),
            math::load(transform.get_rotation()),
            math::load(transform.get_position()));
        math::store(local_matrix, local_transform.matrix);

        transform.clear_local_dirty();

        return local_transform.matrix;
    }

    return world.get_component<const transform_local_component>(e).matrix;
}

mat4f transform_system::get_world_matrix(entity e)
{
    auto& world = get_world();

    std::vector<entity> path = {e};
    while (world.has_component<parent_component>(path.back()))
    {
        const auto& parent = world.get_component<const parent_component>(path.back());
        path.push_back(parent.parent);
    }

    while (!path.empty())
    {
        entity back = path.back();
        path.pop_back();

        if (!world.get_component<const transform_component>(back).is_world_dirty() &&
            !world.get_component<const transform_component>(back).is_local_dirty())
        {
            continue;
        }

        mat4f parent_matrix;
        if (world.has_component<parent_component>(back))
        {
            entity parent = world.get_component<const parent_component>(back).parent;
            parent_matrix = world.get_component<const transform_world_component>(parent).matrix;
        }

        auto& transform = world.get_component<transform_component>(back);

        mat4f_simd local_matrix = math::load(get_local_matrix(back));
        mat4f_simd world_matrix = matrix::mul(local_matrix, math::load(parent_matrix));
        math::store(world_matrix, world.get_component<transform_world_component>(back).matrix);

        transform.clear_world_dirty();

        if (world.has_component<child_component>(back))
        {
            for (const auto& child : world.get_component<const child_component>(back).children)
            {
                world.get_component<transform_component>(child).set_world_dirty();
            }
        }
    }

    return world.get_component<const transform_world_component>(e).matrix;
}

void transform_system::update_local(bool force)
{
    auto& world = get_world();

    world.get_view().read<transform_component>().write<transform_local_component>().each(
        [](const transform_component& transform, transform_local_component& local)
        {
            if (transform.is_local_dirty())
            {
                mat4f_simd local_matrix = matrix::affine_transform(
                    math::load(transform.m_scale),
                    math::load(transform.m_rotation),
                    math::load(transform.m_position));
                math::store(local_matrix, local.matrix);

                transform.clear_local_dirty();
            }
        },
        [this, force](auto& view)
        {
            return force || view.template is_updated<transform_component>(m_system_version);
        });
}

void transform_system::update_world(bool force)
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
            [this, force](auto& view)
            {
                return force ||
                       view.template is_updated<transform_local_component>(m_system_version);
            });

    std::vector<entity> root_entities;

    world.get_view().read<entity>().with<transform_component>().without<parent_component>().each(
        [&root_entities](const entity& e)
        {
            root_entities.push_back(e);
        });

    for (auto& root : root_entities)
    {
        if (world.has_component<child_component>(root))
        {
            mat4f root_matrix = world.get_component<const transform_world_component>(root).matrix;
            for (const auto& child : world.get_component<const child_component>(root).children)
            {
                update_world_recursive(child, root_matrix, force);
            }
        }
    }
}

void transform_system::update_world_recursive(
    entity e,
    const mat4f& parent_world,
    bool parent_dirty)
{
    auto& world = get_world();

    const auto& transform = world.get_component<const transform_component>(e);

    bool need_update = parent_dirty || transform.is_world_dirty() ||
                       world.is_updated<parent_component>(e, m_system_version);

    if (need_update)
    {
        mat4f_simd local_matrix =
            math::load(world.get_component<const transform_local_component>(e).matrix);
        mat4f_simd parent_matrix = math::load(parent_world);

        auto& world_transform = world.get_component<transform_world_component>(e);
        math::store(matrix::mul(local_matrix, parent_matrix), world_transform.matrix);
        transform.clear_world_dirty();

        if (world.has_component<child_component>(e))
        {
            for (const auto& child : world.get_component<const child_component>(e).children)
            {
                update_world_recursive(child, world_transform.matrix, true);
            }
        }
    }
    else
    {
        if (world.has_component<child_component>(e))
        {
            const auto& world_transform = world.get_component<const transform_world_component>(e);
            for (const auto& child : world.get_component<const child_component>(e).children)
            {
                update_world_recursive(child, world_transform.matrix, false);
            }
        }
    }
}
} // namespace violet
#include <algorithm>

#include "components/hierarchy_component.hpp"
#include "components/hierarchy_component_meta.hpp"
#include "math/matrix.hpp"
#include "scene/transform_system.hpp"

namespace violet
{
transform_system::transform_system()
    : system("transform")
{
}

void transform_system::install(application& app) {}

bool transform_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<parent_component>();
    world.register_component<parent_component_meta>();
    world.register_component<child_component>();
    world.register_component<transform_component>();
    world.register_component<transform_local_component>();
    world.register_component<transform_world_component>();

    task_graph& task_graph = get_task_graph();
    task_group& post_update_group = task_graph.get_group("PostUpdate");
    task_group& transform_group =
        task_graph.add_group().set_name("Transform").set_group(post_update_group);

    task_graph.add_task()
        .set_name("Update Transform")
        .set_group(transform_group)
        .set_options(TASK_OPTION_MAIN_THREAD)
        .set_execute(
            [this]()
            {
                update_hierarchy();
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

        mat4f parent_matrix{1.0f};
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

void transform_system::destroy_recursive(entity e)
{
    auto& world = get_world();

    std::queue<entity> queue;
    queue.push(e);

    if (world.has_component<parent_component>(e))
    {
        auto& parent = world.get_component<parent_component>(e);
        auto& children = world.get_component<child_component>(parent.parent).children;

        auto iter = std::ranges::find(children, e);
        std::swap(*iter, children.back());
        children.pop_back();
    }

    while (!queue.empty())
    {
        entity current = queue.front();
        queue.pop();

        if (world.has_component<child_component>(current))
        {
            auto& children = world.get_component<child_component>(current).children;
            for (auto& child : children)
            {
                queue.push(child);
            }
        }

        world.destroy(current);
    }
}

void transform_system::update_hierarchy()
{
    auto& world = get_world();

    std::vector<std::pair<entity, entity>> add_child_entities;
    std::vector<std::pair<entity, entity>> remove_child_entities;

    std::vector<entity> remove_parent_entities;

    world.get_view().read<entity>().read<parent_component>().write<parent_component_meta>().each(
        [&](const entity& e, const parent_component& parent, parent_component_meta& parent_meta)
        {
            if (parent.parent == parent_meta.previous_parent)
            {
                return;
            }

            if (parent.parent != INVALID_ENTITY)
            {
                if (!world.has_component<child_component>(parent.parent))
                {
                    add_child_entities.emplace_back(parent.parent, e);
                }
                else
                {
                    world.get_component<child_component>(parent.parent).children.push_back(e);
                }
            }
            else
            {
                remove_parent_entities.push_back(e);
            }

            if (parent_meta.previous_parent != INVALID_ENTITY)
            {
                remove_child_entities.emplace_back(parent_meta.previous_parent, e);
            }

            if (world.has_component<transform_component>(e))
            {
                world.get_component<transform_component>(e).set_world_dirty();
            }

            parent_meta.previous_parent = parent.parent;
        },
        [this](auto& view)
        {
            return view.template is_updated<parent_component>(m_system_version);
        });

    for (auto& [parent, child] : add_child_entities)
    {
        world.add_component<child_component>(parent);
        world.get_component<child_component>(parent).children.push_back(child);
    }

    for (auto& [parent, child] : remove_child_entities)
    {
        auto& children = world.get_component<child_component>(parent).children;
        auto iter = std::ranges::find(children, child);
        std::swap(*iter, children.back());
        children.pop_back();

        if (children.empty())
        {
            world.remove_component<child_component>(parent);
        }
    }

    for (auto& e : remove_parent_entities)
    {
        world.remove_component<parent_component>(e);
    }
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
                transform.set_world_dirty();
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
        .read<transform_component>()
        .read<transform_local_component>()
        .write<transform_world_component>()
        .without<parent_component>()
        .each(
            [](const transform_component& transform,
               const transform_local_component& local,
               transform_world_component& world)
            {
                world.matrix = local.matrix;
                world.scale = transform.get_scale();
            },
            [this, force](auto& view)
            {
                return force ||
                       view.template is_updated<transform_local_component>(m_system_version);
            });

    std::vector<std::pair<entity, const transform_world_component*>> dirty_entities;

    world.get_view().read<entity>().read<transform_component>().each(
        [&](const entity& e, const transform_component& transform)
        {
            if (!transform.is_world_dirty())
            {
                return;
            }

            if (!world.has_component<parent_component>(e))
            {
                dirty_entities.emplace_back(e, nullptr);
            }
            else
            {
                bool parent_dirty = false;
                entity parent = world.get_component<const parent_component>(e).parent;
                entity current = parent;
                while (world.has_component<transform_component>(current))
                {
                    if (world.get_component<const transform_component>(current).is_world_dirty())
                    {
                        parent_dirty = true;
                        break;
                    }

                    if (!world.has_component<parent_component>(current))
                    {
                        break;
                    }

                    current = world.get_component<const parent_component>(current).parent;
                }

                if (!parent_dirty)
                {
                    const auto& parent_world_transform =
                        world.get_component<const transform_world_component>(parent);
                    dirty_entities.emplace_back(e, &parent_world_transform);
                }
            }
        },
        [this, force](auto& view)
        {
            return force || view.template is_updated<transform_component>(m_system_version);
        });

    for (auto& [entity, parent_world_transform] : dirty_entities)
    {
        update_world_recursive(entity, parent_world_transform);
    }
}

void transform_system::update_world_recursive(
    entity e,
    const transform_world_component* parent_world_transform)
{
    auto& world = get_world();

    const auto& transform = world.get_component<const transform_component>(e);

    transform.clear_world_dirty();

    auto& world_transform = world.get_component<transform_world_component>(e);
    const auto& local_transform = world.get_component<const transform_local_component>(e);

    if (parent_world_transform != nullptr)
    {
        mat4f_simd local_matrix = math::load(local_transform.matrix);
        mat4f_simd parent_matrix = math::load(parent_world_transform->matrix);
        math::store(matrix::mul(local_matrix, parent_matrix), world_transform.matrix);

        vec4f rotation;
        vec3f translation;
        matrix::decompose(world_transform.matrix, world_transform.scale, rotation, translation);
    }
    else
    {
        world_transform.matrix = local_transform.matrix;
        world_transform.scale = transform.get_scale();
    }

    if (world.has_component<child_component>(e))
    {
        for (const auto& child : world.get_component<const child_component>(e).children)
        {
            update_world_recursive(child, &world_transform);
        }
    }
}
} // namespace violet
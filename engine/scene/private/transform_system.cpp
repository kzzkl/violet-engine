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

    get_taskflow()
        .add_task([this]() { update_local(); })
        .set_name("Transform Local Update")
        .add_predecessor("Update");

    return true;
}

void transform_system::update_local()
{
    auto view = get_world().get_view(m_system_version).read<transform>().write<transform_local>();

    view.each(
        [](const transform& transform, transform_local& local)
        {
            matrix4 local_matrix = matrix::affine_transform(
                math::load(transform.scale),
                math::load(transform.rotation),
                math::load(transform.position));
            math::store(local_matrix, local.matrix);
        });

    m_system_version = get_world().get_version();
}

void transform_system::update_world()
{
    /*std::queue<entity> update_queue;

    auto& world = get_world();

    while (!update_queue.empty())
    {
        auto entity = update_queue.front();
        update_queue.pop();

        auto& [parent, children] = world.get_component<hierarchy>(entity);

        matrix4 parent_to_world = math::load(world.get_component<transform_world>(parent).matrix);
        matrix4 local_to_parent = math::load(world.get_component<transform_local>(entity).matrix);

        math::store(
            matrix::mul(local_to_parent, parent_to_world),
            world.get_component<transform_world>(entity).matrix);

        for (auto child : children)
        {
            update_queue.push(child);
        }
    }*/
}
} // namespace violet
#include "physics/physics_system.hpp"
#include "common/hash.hpp"
#include "components/collider_component.hpp"
#include "components/hierarchy_component.hpp"
#include "components/joint_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/rigidbody_meta_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "math/matrix.hpp"
#include "physics_plugin.hpp"

namespace violet
{
physics_system::physics_system()
    : engine_system("physics")
{
}

physics_system::~physics_system() {}

bool physics_system::initialize(const dictionary& config)
{
    m_plugin = std::make_unique<physics_plugin>();
    m_plugin->load(config["plugin"]);
    m_context = std::make_unique<physics_context>(m_plugin->get_plugin());

    task_graph& task_graph = get_task_graph();
    task_group& post_update_group = task_graph.get_group("PostUpdate");
    task_group& transform_group = task_graph.get_group("Transform");
    task_group& physics_group = task_graph.add_group()
                                    .set_name("Physics")
                                    .set_group(post_update_group)
                                    .add_dependency(transform_group);

    task_graph.add_task()
        .set_name("Physics Simulation")
        .set_group(physics_group)
        .set_execute(
            [this]()
            {
                update_rigidbody();
                update_joint();
                simulation();

                m_system_version = get_world().get_version();
            });

    task_graph.get_group("Rendering").add_dependency(physics_group);

    auto& world = get_world();
    world.register_component<rigidbody_component>();
    world.register_component<rigidbody_meta_component>();
    world.register_component<collider_component>();
    world.register_component<joint_component>();

    return true;
}

void physics_system::simulation()
{
    constexpr float time_step = 1.0f / 60.0f;

    m_time += get_timer().get_frame_delta();
    if (m_time < time_step)
    {
        return;
    }

    while (m_time > time_step)
    {
        for (auto& scene : m_scenes)
        {
            if (scene != nullptr)
            {
                scene->simulation(time_step);
            }
        }

        m_time -= time_step;
    }

    auto& world = get_world();

    std::vector<entity> root_entities;

    world.get_view()
        .read<entity>()
        .with<transform_world_component>()
        .without<parent_component>()
        .each(
            [&root_entities](const entity& e)
            {
                root_entities.push_back(e);
            });

    for (entity root : root_entities)
    {
        update_transform(root, {}, false);
    }
}

void physics_system::update_rigidbody()
{
    auto& world = get_world();

    world.get_view()
        .read<rigidbody_component>()
        .read<collider_component>()
        .read<transform_world_component>()
        .write<rigidbody_meta_component>()
        .each(
            [this](
                const rigidbody_component& rigidbody,
                const collider_component& collider,
                const transform_world_component& transform,
                rigidbody_meta_component& rigidbody_meta)
            {
                phy_collision_shape* shape = get_shape(collider.shapes);

                if (rigidbody_meta.rigidbody == nullptr)
                {
                    phy_rigidbody_desc desc = {
                        .type = rigidbody.type,
                        .shape = shape,
                        .mass = rigidbody.mass,
                        .linear_damping = rigidbody.linear_damping,
                        .angular_damping = rigidbody.angular_damping,
                        .restitution = rigidbody.restitution,
                        .friction = rigidbody.friction,
                        .initial_transform = transform.matrix,
                        .activation_state = rigidbody.activation_state,
                        .collision_group = rigidbody.collision_group,
                        .collision_mask = rigidbody.collision_mask,
                    };

                    rigidbody_meta.rigidbody = m_context->create_rigidbody(desc);
                    rigidbody_meta.motion_state = std::make_unique<rigidbody_motion_state>();
                    rigidbody_meta.rigidbody->set_motion_state(rigidbody_meta.motion_state.get());
                }
                else
                {
                    rigidbody_meta.rigidbody->set_mass(rigidbody.mass);
                    rigidbody_meta.rigidbody->set_damping(
                        rigidbody.linear_damping,
                        rigidbody.angular_damping);
                    rigidbody_meta.rigidbody->set_restitution(rigidbody.restitution);
                    rigidbody_meta.rigidbody->set_friction(rigidbody.friction);
                    rigidbody_meta.rigidbody->set_activation_state(rigidbody.activation_state);
                    rigidbody_meta.rigidbody->set_shape(shape);
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<rigidbody_component>(m_system_version) ||
                       view.template is_updated<collider_component>(m_system_version);
            });

    world.get_view()
        .read<scene_component>()
        .read<rigidbody_component>()
        .write<rigidbody_meta_component>()
        .with<transform_world_component>()
        .each(
            [this](
                const scene_component& scene,
                const rigidbody_component& rigidbody,
                rigidbody_meta_component& rigidbody_meta)
            {
                physics_scene* physics_scene = get_scene(scene.layer);
                if (rigidbody_meta.scene == physics_scene)
                {
                    return;
                }

                if (rigidbody_meta.scene != nullptr)
                {
                    rigidbody_meta.scene->remove_rigidbody(rigidbody_meta.rigidbody.get());
                }

                physics_scene->add_rigidbody(rigidbody_meta.rigidbody.get());
                rigidbody_meta.scene = physics_scene;
            },
            [this](auto& view)
            {
                return view.template is_updated<scene_component>(m_system_version);
            });

    world.get_view()
        .read<rigidbody_component>()
        .read<transform_world_component>()
        .write<rigidbody_meta_component>()
        .each(
            [this](
                const rigidbody_component& rigidbody,
                const transform_world_component& transform,
                rigidbody_meta_component& rigidbody_meta)
            {
                if (rigidbody.type == PHY_RIGIDBODY_TYPE_KINEMATIC)
                {
                    mat4f_simd world_matrix = math::load(transform.matrix);
                    mat4f_simd offset_matrix = math::load(rigidbody.offset);

                    mat4f world_transform;
                    math::store(matrix::mul(offset_matrix, world_matrix), world_transform);

                    rigidbody_meta.motion_state->set_transform(world_transform);
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<transform_world_component>(m_system_version);
            });
}

void physics_system::update_joint()
{
    auto& world = get_world();

    world.get_view().read<entity>().write<joint_component>().read<rigidbody_meta_component>().each(
        [&](const entity& e, joint_component& joint, const rigidbody_meta_component& rigidbody_meta)
        {
            for (auto& joint : joint.joints)
            {
                auto* target_rigidbody =
                    world.get_component<rigidbody_meta_component>(joint.target).rigidbody.get();

                if (joint.meta.joint == nullptr)
                {
                    phy_joint_desc desc = {
                        .source = rigidbody_meta.rigidbody.get(),
                        .source_position = joint.source_position,
                        .source_rotation = joint.source_rotation,
                        .target = target_rigidbody,
                        .target_position = joint.target_position,
                        .target_rotation = joint.target_rotation,
                        .min_linear = joint.min_linear,
                        .max_linear = joint.max_linear,
                        .min_angular = joint.min_angular,
                        .max_angular = joint.max_angular,
                    };

                    for (std::size_t i = 0; i < 6; ++i)
                    {
                        desc.spring_enable[i] = joint.spring_enable[i];
                        desc.stiffness[i] = joint.stiffness[i];
                        desc.damping[i] = joint.damping[i];
                    }

                    joint.meta.joint = m_context->create_joint(desc);
                }
                else
                {
                    joint.meta.joint->set_linear(joint.min_linear, joint.max_linear);
                    joint.meta.joint->set_angular(joint.min_angular, joint.max_angular);
                }
            }
        },
        [this](auto& view)
        {
            return view.template is_updated<joint_component>(m_system_version);
        });

    world.get_view()
        .write<joint_component>()
        .read<rigidbody_meta_component>()
        .with<transform_world_component>()
        .each(
            [this](joint_component& joint, const rigidbody_meta_component& rigidbody_meta)
            {
                physics_scene* physics_scene = rigidbody_meta.scene;

                for (auto& joint : joint.joints)
                {
                    if (joint.meta.scene == physics_scene)
                    {
                        continue;
                    }

                    if (joint.meta.scene != nullptr)
                    {
                        joint.meta.scene->remove_joint(joint.meta.joint.get());
                    }

                    physics_scene->add_joint(joint.meta.joint.get());
                    joint.meta.scene = physics_scene;
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<joint_component>(m_system_version);
            });
}

void physics_system::update_transform(entity e, const mat4f& parent_world, bool parent_dirty)
{
    auto& world = get_world();

    bool dirty = parent_dirty;

    if (world.has_component<rigidbody_meta_component>(e))
    {
        auto& rigidbody_meta = world.get_component<rigidbody_meta_component>(e);
        if (rigidbody_meta.motion_state->dirty || parent_dirty)
        {
            mat4f_simd world_matrix = math::load(rigidbody_meta.motion_state->get_transform());
            mat4f_simd parent_matrix = math::load(parent_world);
            mat4f_simd local_matrix =
                matrix::mul(world_matrix, matrix::inverse_transform(parent_matrix));

            vec4f_simd position;
            vec4f_simd rotation;
            vec4f_simd scale;
            matrix::decompose(local_matrix, scale, rotation, position);

            auto& transform = world.get_component<transform_component>(e);
            math::store(position, transform.position);
            math::store(rotation, transform.rotation);
            math::store(scale, transform.scale);

            auto& transform_local = world.get_component<transform_local_component>(e);
            math::store(local_matrix, transform_local.matrix);

            auto& transform_world = world.get_component<transform_world_component>(e);
            math::store(world_matrix, transform_world.matrix);

            dirty = true;
        }

        rigidbody_meta.motion_state->dirty = false;
    }
    else
    {
        if (parent_dirty)
        {
            auto& transform = world.get_component<transform_component>(e);
            mat4f_simd local_matrix = matrix::affine_transform(
                math::load(transform.scale),
                math::load(transform.rotation),
                math::load(transform.position));
            mat4f_simd parent_matrix = math::load(parent_world);
            mat4f_simd world_matrix = matrix::mul(local_matrix, parent_matrix);

            auto& transform_world = world.get_component<transform_world_component>(e);
            math::store(world_matrix, transform_world.matrix);
        }
    }

    if (world.has_component<child_component>(e))
    {
        auto& transform_world = world.get_component<transform_world_component>(e);
        for (const auto& child : world.get_component<const child_component>(e).children)
        {
            update_transform(child, transform_world.matrix, dirty);
        }
    }
}

physics_scene* physics_system::get_scene(std::uint32_t layer)
{
    assert(layer < 64);

    if (layer >= m_scenes.size())
    {
        m_scenes.resize(layer + 1);
    }

    if (m_scenes[layer] == nullptr)
    {
        m_scenes[layer] =
            std::make_unique<physics_scene>(vec3f{0.0f, -9.8f, 0.0f}, nullptr, m_context.get());
    }

    return m_scenes[layer].get();
}

phy_collision_shape* physics_system::get_shape(const std::vector<collider_shape>& shapes)
{
    assert(!shapes.empty());

    auto get_shape_impl = [this](const phy_collision_shape_desc& desc)
    {
        std::uint64_t hash = hash::city_hash_64(&desc, sizeof(phy_collision_shape_desc));
        auto iter = m_shapes.find(hash);
        if (iter != m_shapes.end())
        {
            return iter->second.get();
        }

        auto shape = m_context->create_collision_shape(desc);
        m_shapes[hash] = std::move(shape);
        return m_shapes[hash].get();
    };

    if (shapes.size() == 1)
    {
        return get_shape_impl(shapes[0].shape);
    }

    std::uint64_t hash = hash::city_hash_64(shapes.data(), sizeof(collider_shape) * shapes.size());
    auto iter = m_shapes.find(hash);
    if (iter != m_shapes.end())
    {
        return iter->second.get();
    }

    std::vector<phy_collision_shape*> children(shapes.size());
    std::vector<mat4f> offsets(shapes.size());
    for (const auto& shape : shapes)
    {
        children.push_back(get_shape_impl(shape.shape));
        offsets.push_back(shape.offset);
    }

    auto shape = m_context->create_collision_shape(children, offsets);
    m_shapes[hash] = std::move(shape);

    return m_shapes[hash].get();
}
} // namespace violet
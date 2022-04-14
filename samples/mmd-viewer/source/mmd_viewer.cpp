#include "mmd_viewer.hpp"
#include "animation.hpp"
#include "scene.hpp"
#include "transform.hpp"

namespace ash::sample::mmd
{
bool mmd_viewer::initialize(const dictionary& config)
{
    static const std::vector<std::string> internal_toon_path = {
        "toon01.dds",
        "toon02.dds",
        "toon03.dds",
        "toon04.dds",
        "toon05.dds",
        "toon06.dds",
        "toon07.dds",
        "toon08.dds",
        "toon09.dds",
        "toon10.dds",
    };

    auto& graphics = system<graphics::graphics>();

    for (auto& path : internal_toon_path)
        m_internal_toon.push_back(graphics.make_texture("resource/mmd/" + path));

    m_pipeline = graphics.make_render_pipeline<graphics::render_pipeline>("mmd");

    auto& world = system<ash::ecs::world>();
    world.register_component<mmd_bone>();
    world.register_component<mmd_skeleton>();
    world.register_component<mmd_animation>();

    m_skeleton_view = world.make_view<mmd_skeleton>();
    m_bone_view = world.make_view<mmd_bone, scene::transform>();

    return true;
}

void mmd_viewer::update()
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();

    m_bone_view->each([](mmd_bone& node, scene::transform& transform) {
        transform.position = node.initial_position;
        transform.rotation = node.initial_rotation;
        transform.scaling = node.initial_scale;
    });

    scene.sync_local();

    m_skeleton_view->each([&](mmd_skeleton& skeleton) {
        for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
        {
            math::float4x4_simd to_world =
                math::simd::load(world.component<scene::transform>(skeleton.nodes[i]).world_matrix);
            math::float4x4_simd initial =
                math::simd::load(world.component<mmd_bone>(skeleton.nodes[i]).initial_inverse);

            math::float4x4_simd final_transform = math::matrix_simd::mul(initial, to_world);
            math::simd::store(final_transform, skeleton.transform[i]);
        }

        skeleton.parameter->set(0, skeleton.transform.data(), skeleton.transform.size());
    });
}

void mmd_viewer::update_animation(ecs::entity entity, bool after_physics)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& skeleton = world.component<mmd_skeleton>(entity);

    std::vector<mmd_bone*> nodes;
    nodes.reserve(skeleton.sorted_nodes.size());

    for (auto& node : skeleton.sorted_nodes)
        nodes.push_back(&world.component<mmd_bone>(node));

    for (auto& node_entity : skeleton.nodes)
    {
        auto& node = world.component<mmd_bone>(node_entity);
        auto& node_transform = world.component<scene::transform>(node_entity);

        if (node.deform_after_physics != after_physics)
            continue;

        math::float3 translate =
            math::vector_plain::add(node.animation_translate, node_transform.position);
        if (node.inherit_translation_flag)
            math::vector_plain::add(node.inherit_translate, translate);

        math::float4 rotate =
            math::quaternion_plain::mul(node.animation_rotate, node_transform.rotation);
        if (node.inherit_rotation_flag)
            math::quaternion_plain::mul(node.inherit_rotate, rotate);
    }

    scene.sync_local(entity);

    for (auto& node_entity : skeleton.nodes)
    {
        auto& node = world.component<mmd_bone>(node_entity);

        if (node.deform_after_physics != after_physics)
            continue;

        if (node.inherit_node == ecs::INVALID_ENTITY)
            continue;

        if (node.inherit_rotation_flag)
        {
            
        }
    }
}

ash::ecs::entity mmd_viewer::load_mmd(
    std::string_view name,
    std::string_view pmx,
    std::string_view vmd)
{
    pmx_loader pmx_loader;
    if (!pmx_loader.load(pmx))
        return ecs::INVALID_ENTITY;

    vmd_loader vmd_loader;
    if (!vmd_loader.load(vmd))
        return ecs::INVALID_ENTITY;

    auto& world = system<ash::ecs::world>();
    auto& graphics = system<ash::graphics::graphics>();

    auto& resource = m_resources[name.data()];
    resource.root = world.create();
    world.add<scene::transform, graphics::visual, mmd_skeleton>(resource.root);

    auto& visual = world.component<graphics::visual>(resource.root);
    resource.object_parameter = graphics.make_render_parameter("ash_object");
    visual.object = resource.object_parameter.get();

    load_hierarchy(resource, pmx_loader);
    load_mesh(resource, pmx_loader);
    load_texture(resource, pmx_loader);
    load_material(resource, pmx_loader);
    load_physics(resource, pmx_loader);

    load_animation(resource, pmx_loader, vmd_loader);

    return resource.root;
}

void mmd_viewer::load_hierarchy(mmd_resource& resource, const pmx_loader& loader)
{
    auto& world = system<ash::ecs::world>();
    auto& scene = system<ash::scene::scene>();
    auto& graphics = system<ash::graphics::graphics>();

    auto& skeleton = world.component<mmd_skeleton>(resource.root);
    skeleton.parameter = graphics.make_render_parameter("mmd_skeleton");

    skeleton.nodes.reserve(loader.bones().size());
    for (auto& pmx_bone : loader.bones())
    {
        ecs::entity node_entity = world.create();
        world.add<scene::transform, mmd_bone>(node_entity);

        auto& bone = world.component<mmd_bone>(node_entity);
        bone.name = pmx_bone.name_jp;
        bone.index = static_cast<std::uint32_t>(skeleton.nodes.size());

        skeleton.nodes.push_back(node_entity);
    }

    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        auto& pmx_bone = loader.bones()[i];
        auto& node_entity = skeleton.nodes[i];

        auto& node_transform = world.component<scene::transform>(node_entity);
        node_transform.dirty = true;

        if (pmx_bone.parent_index != -1)
        {
            auto& pmx_parent_bone = loader.bones()[pmx_bone.parent_index];
            auto& parent_node = skeleton.nodes[pmx_bone.parent_index];

            node_transform.position =
                math::vector_plain::sub(pmx_bone.position, pmx_parent_bone.position);

            scene.link(node_entity, parent_node);
        }
        else
        {
            node_transform.position = pmx_bone.position;
            scene.link(node_entity, resource.root);
        }
    }

    scene.sync_local(resource.root);
    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        auto& pmx_bone = loader.bones()[i];
        auto& bone = world.component<mmd_bone>(skeleton.nodes[i]);
        auto& node_transform = world.component<scene::transform>(skeleton.nodes[i]);

        math::float4x4_simd initial = math::simd::load(node_transform.world_matrix);
        math::float4x4_simd inverse = math::matrix_simd::inverse(initial);
        math::simd::store(inverse, bone.initial_inverse);

        bone.layer = pmx_bone.layer;
        bone.deform_after_physics = pmx_bone.flags & pmx_bone_flag::PHYSICS_AFTER_DEFORM;
        bone.inherit_rotation_flag = pmx_bone.flags & pmx_bone_flag::INHERIT_ROTATION;
        bone.inherit_translation_flag = pmx_bone.flags & pmx_bone_flag::INHERIT_TRANSLATION;

        if ((bone.inherit_rotation_flag || bone.inherit_translation_flag) &&
            pmx_bone.inherit_index != -1)
        {
            bone.inherit_local_flag = pmx_bone.flags & pmx_bone_flag::INHERIT_LOCAL;
            bone.inherit_node = skeleton.nodes[pmx_bone.inherit_index];
            bone.inherit_weight = pmx_bone.inherit_weight;
        }

        bone.initial_position = node_transform.position;
        bone.initial_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        bone.initial_scale = {1.0f, 1.0f, 1.0f};
    }

    skeleton.transform.resize(skeleton.nodes.size());

    skeleton.sorted_nodes = skeleton.nodes;
    std::sort(
        skeleton.sorted_nodes.begin(),
        skeleton.sorted_nodes.end(),
        [&](ecs::entity a, ecs::entity b) -> bool {
            return world.component<mmd_bone>(a).layer < world.component<mmd_bone>(b).layer;
        });
}

void mmd_viewer::load_mesh(mmd_resource& resource, const pmx_loader& loader)
{
    struct vertex
    {
        math::float3 position;
        math::float3 normal;
        math::float2 uv;

        math::uint4 bone;
        math::float3 bone_weight;
    };

    auto& graphics = system<ash::graphics::graphics>();

    // Make vertex buffer.
    std::vector<vertex> vertices;
    vertices.reserve(loader.vertices().size());
    for (const pmx_vertex& v : loader.vertices())
        vertices.push_back(vertex{v.position, v.normal, v.uv, v.bone, v.weight});

    resource.vertex_buffer = graphics.make_vertex_buffer(vertices.data(), vertices.size());

    // Make index buffer.
    std::vector<std::int32_t> indices;
    indices.reserve(loader.indices().size());
    for (std::int32_t i : loader.indices())
        indices.push_back(i);

    resource.index_buffer = graphics.make_index_buffer(indices.data(), indices.size());
}

void mmd_viewer::load_texture(mmd_resource& resource, const pmx_loader& loader)
{
    auto& graphics = system<ash::graphics::graphics>();

    for (auto& texture_path : loader.textures())
    {
        std::string dds_path = texture_path.substr(0, texture_path.find_last_of('.')) + ".dds";
        resource.textures.push_back(graphics.make_texture(dds_path));
    }
}

void mmd_viewer::load_material(mmd_resource& resource, const pmx_loader& loader)
{
    auto& graphics = system<ash::graphics::graphics>();
    auto& world = system<ash::ecs::world>();

    for (auto& mmd_material : loader.materials())
    {
        auto parameter = graphics.make_render_parameter("mmd_material");
        parameter->set(0, mmd_material.diffuse);
        parameter->set(1, mmd_material.specular);
        parameter->set(2, mmd_material.specular_strength);
        parameter->set(3, mmd_material.toon_index == -1 ? std::uint32_t(0) : std::uint32_t(1));
        parameter->set(4, static_cast<std::uint32_t>(mmd_material.sphere_mode));
        parameter->set(5, resource.textures[mmd_material.texture_index].get());

        if (mmd_material.toon_index != -1)
        {
            if (mmd_material.toon_mode == toon_mode::TEXTURE)
                parameter->set(6, resource.textures[mmd_material.toon_index].get());
            else if (mmd_material.toon_mode == toon_mode::INTERNAL)
                parameter->set(6, m_internal_toon[mmd_material.toon_index].get());
        }
        if (mmd_material.sphere_mode != sphere_mode::DISABLED)
            parameter->set(7, resource.textures[mmd_material.sphere_index].get());

        resource.materials.push_back(std::move(parameter));
    }

    resource.submesh = loader.submesh();

    auto& visual = world.component<graphics::visual>(resource.root);
    for (std::size_t i = 0; i < resource.submesh.size(); ++i)
    {
        graphics::render_unit s = {};
        s.index_start = resource.submesh[i].first;
        s.index_end = resource.submesh[i].second;
        s.vertex_buffer = resource.vertex_buffer.get();
        s.index_buffer = resource.index_buffer.get();
        s.pipeline = m_pipeline.get();
        s.parameters = {
            visual.object,
            resource.materials[i].get(),
            world.component<mmd_skeleton>(resource.root).parameter.get()};

        visual.submesh.push_back(s);
    }
}

void mmd_viewer::load_ik(mmd_resource& resource, const pmx_loader& loader)
{
    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        auto& pmx_bone = loader.bones()[i];
        if (pmx_bone.flags & pmx_bone_flag::IK)
        {
        }
    }
}

void mmd_viewer::load_physics(mmd_resource& resource, const pmx_loader& loader)
{
    auto& world = system<ash::ecs::world>();
    auto& physics = system<ash::physics::physics>();

    auto& skeleton = world.component<mmd_skeleton>(resource.root);

    std::vector<ecs::entity> physics_nodes;
    for (auto& pmx_rigidbody : loader.rigidbodies())
    {
        physics::collision_shape_desc desc = {};
        switch (pmx_rigidbody.shape)
        {
        case pmx_rigidbody_shape_type::SPHERE:
            desc.type = physics::collision_shape_type::SPHERE;
            desc.sphere.radius = pmx_rigidbody.size[0];
            break;
        case pmx_rigidbody_shape_type::BOX:
            desc.type = physics::collision_shape_type::BOX;
            desc.box.length = pmx_rigidbody.size[0] * 2.0f;
            desc.box.height = pmx_rigidbody.size[1] * 2.0f;
            desc.box.width = pmx_rigidbody.size[2] * 2.0f;
            break;
        case pmx_rigidbody_shape_type::CAPSULE:
            desc.type = physics::collision_shape_type::CAPSULE;
            desc.capsule.radius = pmx_rigidbody.size[0];
            desc.capsule.height = pmx_rigidbody.size[1];
            break;
        default:
            break;
        }
        resource.collision_shapes.push_back(physics.make_shape(desc));

        ecs::entity node_entity;
        if (pmx_rigidbody.bone_index != -1)
            node_entity = skeleton.nodes[pmx_rigidbody.bone_index];
        else
            node_entity = resource.root;

        if (!world.has_component<physics::rigidbody>(node_entity))
            world.add<physics::rigidbody>(node_entity);
        physics_nodes.push_back(node_entity);
        auto& rigidbody = world.component<physics::rigidbody>(node_entity);

        rigidbody.shape(resource.collision_shapes.back().get());
        rigidbody.mass(
            pmx_rigidbody.mode == pmx_rigidbody_mode::STATIC ? 0.0f : pmx_rigidbody.mass);
        rigidbody.linear_dimmer(pmx_rigidbody.translate_dimmer);
        rigidbody.angular_dimmer(pmx_rigidbody.rotate_dimmer);
        rigidbody.restitution(pmx_rigidbody.repulsion);
        rigidbody.friction(pmx_rigidbody.friction);

        switch (pmx_rigidbody.mode)
        {
        case pmx_rigidbody_mode::STATIC:
            rigidbody.type(physics::rigidbody_type::KINEMATIC);
            break;
        case pmx_rigidbody_mode::DYNAMIC:
            rigidbody.type(physics::rigidbody_type::DYNAMIC);
            break;
        case pmx_rigidbody_mode::MERGE:
            rigidbody.type(physics::rigidbody_type::KINEMATIC);
            break;
        default:
            break;
        }

        math::float4_simd position_offset = math::simd::load(pmx_rigidbody.translate);
        math::float4_simd rotation_offset = math::simd::load(pmx_rigidbody.rotate);
        math::float4x4_simd rigidbody_world = math::matrix_simd::affine_transform(
            math::simd::set(1.0f, 1.0f, 1.0f, 0.0f),
            rotation_offset,
            position_offset);
        math::float4x4_simd node_offset =
            math::simd::load(world.component<scene::transform>(node_entity).world_matrix);
        node_offset = math::matrix_simd::inverse(node_offset);

        math::float4x4 offset;
        math::simd::store(math::matrix_simd::mul(rigidbody_world, node_offset), offset);

        rigidbody.offset(offset);
        rigidbody.collision_group(1 << pmx_rigidbody.group);
        rigidbody.collision_mask(pmx_rigidbody.collision_group);
    }

    for (auto& pmx_joint : loader.joints())
    {
        auto rigidbody_a = physics_nodes[pmx_joint.rigidbody_a_index];
        auto rigidbody_b = physics_nodes[pmx_joint.rigidbody_b_index];

        if (!world.has_component<physics::joint>(rigidbody_a))
            world.add<physics::joint>(rigidbody_a);

        auto& joint = world.component<physics::joint>(rigidbody_a);

        std::size_t index = joint.add_unit();
        joint.rigidbody(index, rigidbody_b);
        joint.location(index, pmx_joint.translate);
        joint.rotation(
            index,
            math::quaternion_plain::rotation_euler(
                pmx_joint.rotate[1],
                pmx_joint.rotate[0],
                pmx_joint.rotate[2]));
        joint.min_linear(index, pmx_joint.translate_min);
        joint.max_linear(index, pmx_joint.translate_max);
        joint.min_angular(index, pmx_joint.rotate_min);
        joint.max_angular(index, pmx_joint.rotate_max);
        joint.spring_translate_factor(index, pmx_joint.spring_translate_factor);
        joint.spring_rotate_factor(index, pmx_joint.spring_rotate_factor);
    }
}

void mmd_viewer::load_animation(
    mmd_resource& resource,
    const pmx_loader& pmx_loader,
    const vmd_loader& vmd_loader)
{
    auto& world = system<ecs::world>();
    auto& skeletion = world.component<mmd_skeleton>(resource.root);

    std::map<std::string, mmd_animation*> animation_map;
    for (auto& node_entity : skeletion.nodes)
    {
        world.add<mmd_animation>(node_entity);
        auto& animation = world.component<mmd_animation>(node_entity);

        auto& bone = world.component<mmd_bone>(node_entity);
        animation_map[bone.name] = &animation;
    }

    auto set_bezier = [](math::float4& bezier, const unsigned char* cp) {
        int x0 = cp[0];
        int y0 = cp[4];
        int x1 = cp[8];
        int y1 = cp[12];

        bezier = {(float)x0 / 127.0f, (float)y0 / 127.0f, (float)x1 / 127.0f, (float)y1 / 127.0f};
    };

    for (auto& pmx_motion : vmd_loader.motions())
    {
        auto iter = animation_map.find(pmx_motion.bone_name);
        if (iter != animation_map.end())
        {
            mmd_animation_key key;
            key.frame = pmx_motion.frame_index;
            key.translate = pmx_motion.translate;
            key.rotate = pmx_motion.rotate;

            set_bezier(key.tx_bezier, &pmx_motion.interpolation[0]);
            set_bezier(key.ty_bezier, &pmx_motion.interpolation[1]);
            set_bezier(key.tz_bezier, &pmx_motion.interpolation[2]);
            set_bezier(key.r_bezier, &pmx_motion.interpolation[3]);

            iter->second->keys.push_back(key);
        }
    }

    for (auto [key, value] : animation_map)
    {
        std::sort(
            value->keys.begin(),
            value->keys.end(),
            [](const mmd_animation_key& a, const mmd_animation_key& b) {
                return a.frame < b.frame;
            });
    }
}
} // namespace ash::sample::mmd
#include "mmd_loader.hpp"
#include <iostream>

namespace ash::sample::mmd
{
mmd_loader::mmd_loader()
{
}

void mmd_loader::initialize()
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
}

bool mmd_loader::load(
    ecs::entity entity,
    mmd_resource& resource,
    std::string_view pmx,
    std::string_view vmd,
    graphics::render_pipeline* render_pipeline,
    graphics::skin_pipeline* skin_pipeline)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();

    pmx_loader pmx_loader;
    if (!pmx_loader.load(pmx))
        return false;

    vmd_loader vmd_loader;
    if (!vmd_loader.load(vmd))
        return false;

    world.add<core::link, scene::transform, graphics::visual, graphics::skinned_mesh, mmd_skeleton>(
        entity);

    auto& visual = world.component<graphics::visual>(entity);
    resource.object_parameter = graphics.make_pipeline_parameter("ash_object");
    visual.object = resource.object_parameter.get();

    auto& skinned_mesh = world.component<graphics::skinned_mesh>(entity);
    skinned_mesh.pipeline = skin_pipeline;
    skinned_mesh.parameter = graphics.make_pipeline_parameter("mmd_skin");

    load_hierarchy(entity, resource, pmx_loader);
    load_mesh(entity, resource, pmx_loader);
    load_texture(entity, resource, pmx_loader);
    load_material(entity, resource, pmx_loader, render_pipeline);
    load_physics(entity, resource, pmx_loader);
    load_ik(entity, resource, pmx_loader);
    load_animation(entity, resource, pmx_loader, vmd_loader);

    return true;
}

void mmd_loader::load_hierarchy(
    ecs::entity entity,
    mmd_resource& resource,
    const pmx_loader& loader)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();
    auto& relation = system<core::relation>();
    auto& scene = system<scene::scene>();

    auto& skeleton = world.component<mmd_skeleton>(entity);
    skeleton.nodes.reserve(loader.bones().size());
    for (auto& pmx_bone : loader.bones())
    {
        ecs::entity node_entity = world.create("node");
        world.add<core::link, scene::transform, mmd_node>(node_entity);

        auto& bone = world.component<mmd_node>(node_entity);
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

            relation.link(node_entity, parent_node);
        }
        else
        {
            node_transform.position = pmx_bone.position;
            relation.link(node_entity, entity);
        }
    }

    scene.sync_local(entity);
    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        auto& pmx_bone = loader.bones()[i];
        auto& bone = world.component<mmd_node>(skeleton.nodes[i]);
        auto& node_transform = world.component<scene::transform>(skeleton.nodes[i]);

        math::float4x4_simd initial = math::simd::load(node_transform.world_matrix);
        math::float4x4_simd inverse = math::matrix_simd::inverse(initial);
        math::simd::store(inverse, bone.initial_inverse);

        bone.layer = pmx_bone.layer;
        bone.deform_after_physics = pmx_bone.flags & pmx_bone_flag::PHYSICS_AFTER_DEFORM;
        bone.is_inherit_rotation = pmx_bone.flags & pmx_bone_flag::INHERIT_ROTATION;
        bone.is_inherit_translation = pmx_bone.flags & pmx_bone_flag::INHERIT_TRANSLATION;

        if ((bone.is_inherit_rotation || bone.is_inherit_translation) &&
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

    skeleton.local.resize(skeleton.nodes.size());
    skeleton.world.resize(skeleton.nodes.size());

    skeleton.sorted_nodes = skeleton.nodes;
    std::stable_sort(
        skeleton.sorted_nodes.begin(),
        skeleton.sorted_nodes.end(),
        [&](ecs::entity a, ecs::entity b) -> bool {
            return world.component<mmd_node>(a).layer < world.component<mmd_node>(b).layer;
        });
}

void mmd_loader::load_mesh(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();

    // Make vertex buffer.
    std::vector<math::float3> position;
    std::vector<math::float3> normal;
    std::vector<math::float2> uv;
    std::vector<math::uint4> bone;
    std::vector<math::float3> bone_weight;

    position.reserve(loader.vertices().size());
    normal.reserve(loader.vertices().size());
    uv.reserve(loader.vertices().size());
    bone.reserve(loader.vertices().size());
    bone_weight.reserve(loader.vertices().size());
    for (const pmx_vertex& v : loader.vertices())
    {
        position.push_back(v.position);
        normal.push_back(v.normal);
        uv.push_back(v.uv);
        bone.push_back(v.bone);
        bone_weight.push_back(v.weight);
    }

    enum mmd_vertex_attribute
    {
        MMD_VERTEX_ATTRIBUTE_POSITION,
        MMD_VERTEX_ATTRIBUTE_NORMAL,
        MMD_VERTEX_ATTRIBUTE_UV,
        MMD_VERTEX_ATTRIBUTE_BONE,
        MMD_VERTEX_ATTRIBUTE_BONE_WEIGHT,
        MMD_VERTEX_ATTRIBUTE_NUM_TYPES
    };
    resource.vertex_buffers.resize(MMD_VERTEX_ATTRIBUTE_NUM_TYPES);

    resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_POSITION] = graphics.make_vertex_buffer(
        position.data(),
        position.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_NORMAL] = graphics.make_vertex_buffer(
        normal.data(),
        normal.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_UV] =
        graphics.make_vertex_buffer(uv.data(), uv.size());
    resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_BONE] =
        graphics.make_vertex_buffer(bone.data(), bone.size(), graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);
    resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_BONE_WEIGHT] = graphics.make_vertex_buffer(
        bone_weight.data(),
        bone_weight.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN);

    auto& visual = world.component<graphics::visual>(entity);
    visual.vertex_buffers = {
        resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_POSITION].get(),
        resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_NORMAL].get(),
        resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_UV].get()};

    auto& skinned_mesh = world.component<graphics::skinned_mesh>(entity);
    skinned_mesh.input_vertex_buffers = {
        resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_POSITION].get(),
        resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_NORMAL].get(),
        resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_BONE].get(),
        resource.vertex_buffers[MMD_VERTEX_ATTRIBUTE_BONE_WEIGHT].get()};

    skinned_mesh.skinned_vertex_buffers.push_back(graphics.make_vertex_buffer(
        position.data(),
        position.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_OUT));
    skinned_mesh.skinned_vertex_buffers.push_back(graphics.make_vertex_buffer(
        normal.data(),
        normal.size(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_OUT));
    skinned_mesh.skinned_vertex_buffers.resize(visual.vertex_buffers.size());
    skinned_mesh.vertex_count = loader.vertices().size();

    // Make index buffer.
    std::vector<std::int32_t> indices;
    indices.reserve(loader.indices().size());
    for (std::int32_t i : loader.indices())
        indices.push_back(i);

    resource.index_buffer = graphics.make_index_buffer(indices.data(), indices.size());
    visual.index_buffer = resource.index_buffer.get();
}

void mmd_loader::load_texture(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader)
{
    auto& graphics = system<graphics::graphics>();

    for (auto& texture_path : loader.textures())
    {
        std::string dds_path = texture_path.substr(0, texture_path.find_last_of('.')) + ".dds";
        resource.textures.push_back(graphics.make_texture(dds_path));
    }
}

void mmd_loader::load_material(
    ecs::entity entity,
    mmd_resource& resource,
    const pmx_loader& loader,
    graphics::render_pipeline* render_pipeline)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();

    for (auto& mmd_material : loader.materials())
    {
        auto parameter = graphics.make_pipeline_parameter("mmd_material");
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
            else
                throw std::runtime_error("Invalid toon mode");
        }
        if (mmd_material.sphere_mode != sphere_mode::DISABLED)
            parameter->set(7, resource.textures[mmd_material.sphere_index].get());

        resource.materials.push_back(std::move(parameter));
    }

    resource.submesh = loader.submesh();

    auto& visual = world.component<graphics::visual>(entity);
    for (std::size_t i = 0; i < resource.submesh.size(); ++i)
    {
        graphics::submesh mesh = {};
        mesh.index_start = resource.submesh[i].first;
        mesh.index_end = resource.submesh[i].second;
        visual.submeshes.push_back(mesh);

        graphics::material material = {};
        material.pipeline = render_pipeline;
        material.parameters = {visual.object, resource.materials[i].get()};
        visual.materials.push_back(material);
    }
}

void mmd_loader::load_ik(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader)
{
    auto& world = system<ecs::world>();

    auto& skeleton = world.component<mmd_skeleton>(entity);

    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        auto& pmx_bone = loader.bones()[i];
        if (pmx_bone.flags & pmx_bone_flag::IK)
        {
            auto& ik_entity = skeleton.nodes[i];

            world.add<mmd_ik_solver>(ik_entity);
            auto& solver = world.component<mmd_ik_solver>(ik_entity);
            solver.loop_count = pmx_bone.ik_loop_count;
            solver.limit_angle = pmx_bone.ik_limit;
            solver.ik_target = skeleton.nodes[pmx_bone.ik_target_index];

            for (auto& pmx_ik_link : pmx_bone.ik_links)
            {
                auto& link_entity = skeleton.nodes[pmx_ik_link.bone_index];

                world.add<mmd_ik_link>(link_entity);
                auto& link = world.component<mmd_ik_link>(link_entity);

                if (pmx_ik_link.enable_limit)
                {
                    link.enable_limit = true;
                    link.limit_min = pmx_ik_link.limit_min;
                    link.limit_max = pmx_ik_link.limit_max;
                    link.save_ik_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
                }
                else
                {
                    link.enable_limit = false;
                    link.save_ik_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
                }

                solver.links.push_back(link_entity);
            }
        }
    }
}

void mmd_loader::load_physics(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader)
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& physics = system<physics::physics>();

    auto& skeleton = world.component<mmd_skeleton>(entity);

    std::vector<math::float4x4> rigidbody_transform;
    rigidbody_transform.reserve(loader.rigidbodies().size());

    auto make_shape = [&](const pmx_rigidbody& pmx_rigidbody)
        -> std::unique_ptr<physics::collision_shape_interface> {
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
            return nullptr;
        };
        return physics.make_shape(desc);
    };

    for (auto& pmx_rigidbody : loader.rigidbodies())
    {
        rigidbody_transform.push_back(math::matrix_plain::affine_transform(
            math::float3{1.0f, 1.0f, 1.0f},
            pmx_rigidbody.rotate,
            pmx_rigidbody.translate));

        ecs::entity node = skeleton.nodes[pmx_rigidbody.bone_index];
        if (!world.has_component<physics::rigidbody>(node))
        {
            world.add<physics::rigidbody>(node);
        }
        else
        {
            // Workaround multiple rigid bodies are attached to a node.
            ecs::entity workaround_node = world.create("rigidbody");
            world.add<core::link, physics::rigidbody, scene::transform>(workaround_node);
            relation.link(workaround_node, node);
            node = workaround_node;
            system<scene::scene>().sync_local(node);
        }

        resource.collision_shapes.push_back(make_shape(pmx_rigidbody));

        auto& rigidbody = world.component<physics::rigidbody>(node);
        rigidbody.shape = resource.collision_shapes.back().get();

        switch (pmx_rigidbody.mode)
        {
        case pmx_rigidbody_mode::STATIC:
            rigidbody.type = physics::rigidbody_type::KINEMATIC;
            break;
        case pmx_rigidbody_mode::DYNAMIC:
            rigidbody.type = physics::rigidbody_type::DYNAMIC;
            break;
        case pmx_rigidbody_mode::MERGE:
            rigidbody.type = physics::rigidbody_type::KINEMATIC;
            break;
        default:
            break;
        }

        rigidbody.mass =
            pmx_rigidbody.mode == pmx_rigidbody_mode::STATIC ? 0.0f : pmx_rigidbody.mass;
        rigidbody.linear_dimmer = pmx_rigidbody.translate_dimmer;
        rigidbody.angular_dimmer = pmx_rigidbody.rotate_dimmer;
        rigidbody.restitution = pmx_rigidbody.repulsion;
        rigidbody.friction = pmx_rigidbody.friction;
        rigidbody.collision_group = 1 << pmx_rigidbody.group;
        rigidbody.collision_mask = pmx_rigidbody.collision_group;

        rigidbody.offset = math::matrix_plain::mul(
            math::matrix_plain::affine_transform(
                math::float3{1.0f, 1.0f, 1.0f},
                pmx_rigidbody.rotate,
                pmx_rigidbody.translate),
            math::matrix_plain::inverse(world.component<scene::transform>(node).world_matrix));
    }

    ecs::entity joint_group = world.create("joints");
    world.add<core::link>(joint_group);
    for (auto& pmx_joint : loader.joints())
    {
        ecs::entity joint_entity = world.create("joint");
        world.add<core::link, physics::joint>(joint_entity);
        auto& joint = world.component<physics::joint>(joint_entity);

        joint.relation_a =
            skeleton.nodes[loader.rigidbodies()[pmx_joint.rigidbody_a_index].bone_index];
        joint.relation_b =
            skeleton.nodes[loader.rigidbodies()[pmx_joint.rigidbody_b_index].bone_index];
        joint.min_linear = pmx_joint.translate_min;
        joint.max_linear = pmx_joint.translate_max;
        joint.min_angular = pmx_joint.rotate_min;
        joint.max_angular = pmx_joint.rotate_max;
        joint.spring_translate_factor = pmx_joint.spring_translate_factor;
        joint.spring_rotate_factor = pmx_joint.spring_rotate_factor;

        math::float4_simd position = math::simd::load(pmx_joint.translate);
        math::float4_simd rotation = math::simd::load(pmx_joint.rotate);
        math::float4x4_simd joint_world = math::matrix_simd::affine_transform(
            math::simd::set(1.0f, 1.0f, 1.0f, 0.0f),
            rotation,
            position);

        math::float4x4_simd inverse_a = math::matrix_simd::inverse(
            math::simd::load(rigidbody_transform[pmx_joint.rigidbody_a_index]));
        math::float4x4_simd offset_a = math::matrix_simd::mul(joint_world, inverse_a);

        math::float4x4_simd inverse_b = math::matrix_simd::inverse(
            math::simd::load(rigidbody_transform[pmx_joint.rigidbody_b_index]));
        math::float4x4_simd offset_b = math::matrix_simd::mul(joint_world, inverse_b);

        math::float4_simd scale;
        math::matrix_simd::decompose(offset_a, scale, rotation, position);
        math::simd::store(position, joint.relative_position_a);
        math::simd::store(rotation, joint.relative_rotation_a);

        math::matrix_simd::decompose(offset_b, scale, rotation, position);
        math::simd::store(position, joint.relative_position_b);
        math::simd::store(rotation, joint.relative_rotation_b);

        relation.link(joint_entity, joint_group);
    }
    relation.link(joint_group, entity);
}

void mmd_loader::load_animation(
    ecs::entity entity,
    mmd_resource& resource,
    const pmx_loader& pmx_loader,
    const vmd_loader& vmd_loader)
{
    auto& world = system<ecs::world>();

    auto& skeletion = world.component<mmd_skeleton>(entity);

    // Node.
    std::map<std::string, mmd_node_animation*> node_map;
    for (auto& node_entity : skeletion.nodes)
    {
        world.add<mmd_node_animation>(node_entity);
        auto& animation = world.component<mmd_node_animation>(node_entity);

        auto& bone = world.component<mmd_node>(node_entity);
        node_map[bone.name] = &animation;
    }

    auto set_bezier = [](mmd_bezier& bezier, const unsigned char* cp) {
        int x0 = cp[0];
        int y0 = cp[4];
        int x1 = cp[8];
        int y1 = cp[12];

        bezier.set(
            {(float)x0 / 127.0f, (float)y0 / 127.0f},
            {(float)x1 / 127.0f, (float)y1 / 127.0f});
    };

    for (auto& pmx_motion : vmd_loader.motions())
    {
        auto iter = node_map.find(pmx_motion.bone_name);
        if (iter != node_map.end())
        {
            mmd_node_animation::key key;
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

    for (auto [key, value] : node_map)
    {
        std::sort(value->keys.begin(), value->keys.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
        });
    }

    // IK.
    // TODO
    /*std::map<std::string, mmd_ik_animation*> ik_map;
    for (auto& ik : vmd_loader.iks())
    {
        for (auto& info : ik.infos)
        {
            auto iter = ik_map.find(info.name);
            if (iter != ik_map.end())
            {
            }
        }
    }*/
}
} // namespace ash::sample::mmd
#include "mmd_loader.hpp"
#include "core/relation.hpp"
#include "graphics/rhi.hpp"
#include "mmd_component.hpp"
#include "physics/physics.hpp"
#include "scene/scene.hpp"

namespace violet::sample::mmd
{
class mmd_merge_rigidbody_transform_reflection : public physics::rigidbody_transform_reflection
{
public:
    virtual void reflect(
        const math::float4x4& rigidbody_transform,
        const physics::rigidbody& rigidbody,
        scene::transform& transform) const noexcept override
    {
        math::float4x4_simd to_world = math::simd::load(rigidbody_transform);
        math::float4x4_simd offset_inverse = math::simd::load(rigidbody.offset_inverse());
        to_world = math::matrix_simd::mul(offset_inverse, to_world);
        to_world[3] = math::simd::load(transform.to_world()[3]);
        transform.to_world(to_world);
    }
};

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

    for (auto& path : internal_toon_path)
        m_internal_toon.push_back(graphics::rhi::make_texture("mmd-viewer/mmd/" + path));
}

bool mmd_loader::load(
    ecs::entity entity,
    std::string_view pmx,
    std::string_view vmd,
    graphics::render_pipeline* render_pipeline,
    graphics::skinning_pipeline* skinning_pipeline)
{
    auto& world = system<ecs::world>();
    bool static_model = vmd.empty();

    if (static_model)
        world.add<core::link, scene::transform, graphics::mesh_render, mmd_skeleton>(entity);
    else
        world.add<
            core::link,
            scene::transform,
            graphics::mesh_render,
            graphics::skinned_mesh,
            mmd_skeleton>(entity);

    auto& mesh_render = world.component<graphics::mesh_render>(entity);
    mesh_render.object_parameter = std::make_unique<graphics::object_pipeline_parameter>();

    pmx_loader& pmx_loader = m_pmx[pmx.data()];
    load_hierarchy(entity, pmx_loader);
    load_mesh(entity, pmx_loader, static_model ? nullptr : skinning_pipeline);
    load_material(entity, pmx_loader, render_pipeline);
    load_physics(entity, pmx_loader);
    load_ik(entity, pmx_loader);

    if (!static_model)
    {
        vmd_loader& vmd_loader = m_vmd[vmd.data()];
        load_morph(entity, pmx_loader, vmd_loader);
        load_animation(entity, pmx_loader, vmd_loader);
    }

    return true;
}

bool mmd_loader::load_pmx(std::string_view pmx)
{
    pmx_loader& pmx_loader = m_pmx[pmx.data()];

    std::vector<graphics::resource_interface*> internal_toon;
    for (auto& toon : m_internal_toon)
        internal_toon.push_back(toon.get());
    return pmx_loader.load(pmx, internal_toon);
}

bool mmd_loader::load_vmd(std::string_view vmd)
{
    vmd_loader& vmd_loader = m_vmd[vmd.data()];
    return vmd_loader.load(vmd);
}

void mmd_loader::load_hierarchy(ecs::entity entity, const pmx_loader& loader)
{
    auto& world = system<ecs::world>();
    auto& relation = system<core::relation>();
    auto& scene = system<scene::scene>();

    auto& skeleton = world.component<mmd_skeleton>(entity);
    skeleton.nodes.reserve(loader.bones().size());
    for (auto& pmx_bone : loader.bones())
    {
        ecs::entity node_entity = world.create("node");
        world.add<core::link, scene::transform, mmd_node, mmd_node_animation>(node_entity);

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

        if (pmx_bone.parent_index != -1)
        {
            auto& pmx_parent_bone = loader.bones()[pmx_bone.parent_index];
            auto& parent_node = skeleton.nodes[pmx_bone.parent_index];

            node_transform.position(math::vector::sub(pmx_bone.position, pmx_parent_bone.position));

            relation.link(node_entity, parent_node);
        }
        else
        {
            node_transform.position(pmx_bone.position);
            relation.link(node_entity, entity);
        }
    }

    scene.sync_local(entity);
    for (std::size_t i = 0; i < loader.bones().size(); ++i)
    {
        auto& pmx_bone = loader.bones()[i];
        auto& bone = world.component<mmd_node>(skeleton.nodes[i]);
        auto& node_transform = world.component<scene::transform>(skeleton.nodes[i]);

        math::float4x4_simd initial = math::simd::load(node_transform.to_world());
        math::float4x4_simd inverse = math::matrix_simd::inverse_transform_no_scale(initial);
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

        bone.initial_position = node_transform.position();
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

void mmd_loader::load_mesh(
    ecs::entity entity,
    const pmx_loader& loader,
    graphics::skinning_pipeline* skinning_pipeline)
{
    auto& world = system<ecs::world>();

    auto& mesh_render = world.component<graphics::mesh_render>(entity);
    mesh_render.vertex_buffers = {
        loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_POSITION),
        loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_NORMAL),
        loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_UV),
        loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_EDGE)};
    mesh_render.index_buffer = loader.index_buffer();

    if (skinning_pipeline != nullptr)
    {
        auto& skinned_mesh = world.component<graphics::skinned_mesh>(entity);
        skinned_mesh.pipeline = skinning_pipeline;
        skinned_mesh.skinned_vertex_buffers.resize(mesh_render.vertex_buffers.size());
        skinned_mesh.skinned_vertex_buffers[PMX_VERTEX_ATTRIBUTE_POSITION] =
            graphics::rhi::make_vertex_buffer<math::float3>(
                nullptr,
                loader.vertex_count(),
                graphics::VERTEX_BUFFER_FLAG_COMPUTE_OUT);
        skinned_mesh.skinned_vertex_buffers[PMX_VERTEX_ATTRIBUTE_NORMAL] =
            graphics::rhi::make_vertex_buffer<math::float3>(
                nullptr,
                loader.vertex_count(),
                graphics::VERTEX_BUFFER_FLAG_COMPUTE_OUT);
        skinned_mesh.skinned_vertex_buffers[PMX_VERTEX_ATTRIBUTE_UV] =
            graphics::rhi::make_vertex_buffer<math::float2>(
                nullptr,
                loader.vertex_count(),
                graphics::VERTEX_BUFFER_FLAG_COMPUTE_OUT);
        skinned_mesh.vertex_count = loader.vertex_count();

        auto skin_parameter = std::make_unique<skinning_pipeline_parameter>();
        skin_parameter->input_position(loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_POSITION));
        skin_parameter->input_normal(loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_NORMAL));
        skin_parameter->input_uv(loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_UV));

        skin_parameter->skin(loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_SKIN));
        skin_parameter->bdef_bone(loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_BDEF_BONE));
        skin_parameter->sdef_bone(loader.vertex_buffers(PMX_VERTEX_ATTRIBUTE_SDEF_BONE));

        skin_parameter->output_position(
            skinned_mesh.skinned_vertex_buffers[PMX_VERTEX_ATTRIBUTE_POSITION].get());
        skin_parameter->output_normal(
            skinned_mesh.skinned_vertex_buffers[PMX_VERTEX_ATTRIBUTE_NORMAL].get());
        skin_parameter->output_uv(
            skinned_mesh.skinned_vertex_buffers[PMX_VERTEX_ATTRIBUTE_UV].get());
        skinned_mesh.parameter = std::move(skin_parameter);
    }
}

void mmd_loader::load_material(
    ecs::entity entity,
    const pmx_loader& loader,
    graphics::render_pipeline* render_pipeline)
{
    auto& world = system<ecs::world>();
    auto& mesh_render = world.component<graphics::mesh_render>(entity);

    auto& submesh = loader.submesh();
    for (std::size_t i = 0; i < submesh.size(); ++i)
    {
        graphics::submesh mesh = {};
        mesh.index_start = submesh[i].first;
        mesh.index_end = submesh[i].second;
        mesh_render.submeshes.push_back(mesh);

        graphics::material material;
        material.pipeline = render_pipeline;
        material.parameter = loader.materials(i)->interface();
        mesh_render.materials.push_back(material);
    }
}

void mmd_loader::load_ik(ecs::entity entity, const pmx_loader& loader)
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

void mmd_loader::load_physics(ecs::entity entity, const pmx_loader& loader)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();
    auto& physics = system<physics::physics>();

    auto& skeleton = world.component<mmd_skeleton>(entity);

    std::vector<std::size_t> rigidbody_count(loader.bones().size());
    std::vector<math::float4x4> rigidbody_transform;
    rigidbody_transform.reserve(loader.rigidbodies().size());
    for (auto& pmx_rigidbody : loader.rigidbodies())
    {
        rigidbody_transform.push_back(math::matrix::affine_transform(
            math::float3{1.0f, 1.0f, 1.0f},
            pmx_rigidbody.rotate,
            pmx_rigidbody.translate));
        ++rigidbody_count[pmx_rigidbody.bone_index];
    }

    std::vector<ecs::entity> rigidbody_nodes;
    rigidbody_nodes.reserve(loader.rigidbodies().size());
    for (std::size_t i = 0; i < loader.rigidbodies().size(); ++i)
    {
        auto& pmx_rigidbody = loader.rigidbodies()[i];

        ecs::entity node = skeleton.nodes[pmx_rigidbody.bone_index];
        if (rigidbody_count[pmx_rigidbody.bone_index] == 1)
        {
            world.add<physics::rigidbody>(node);
        }
        else
        {
            // Workaround multiple rigid bodies are attached to a node.
            ecs::entity workaround_node = world.create("rigidbody");
            world.add<core::link, physics::rigidbody, scene::transform>(workaround_node);
            relation.link(workaround_node, node);
            scene.sync_local(workaround_node);

            node = workaround_node;
        }
        rigidbody_nodes.push_back(node);

        auto& rigidbody = world.component<physics::rigidbody>(node);
        rigidbody.shape(loader.collision_shape(i));

        switch (pmx_rigidbody.mode)
        {
        case pmx_rigidbody_mode::STATIC:
            rigidbody.type(physics::rigidbody_type::KINEMATIC);
            break;
        case pmx_rigidbody_mode::DYNAMIC:
            rigidbody.type(physics::rigidbody_type::DYNAMIC);
            break;
        case pmx_rigidbody_mode::MERGE:
            rigidbody.type(physics::rigidbody_type::DYNAMIC);
            rigidbody.reflection<mmd_merge_rigidbody_transform_reflection>();
            break;
        default:
            break;
        }

        rigidbody.mass(
            pmx_rigidbody.mode == pmx_rigidbody_mode::STATIC ? 0.0f : pmx_rigidbody.mass);
        rigidbody.linear_damping(pmx_rigidbody.linear_damping);
        rigidbody.angular_damping(pmx_rigidbody.angular_damping);
        rigidbody.restitution(pmx_rigidbody.repulsion);
        rigidbody.friction(pmx_rigidbody.friction);
        rigidbody.collision_group(1 << pmx_rigidbody.group);
        rigidbody.collision_mask(pmx_rigidbody.collision_group);
        rigidbody.offset(math::matrix::mul(
            rigidbody_transform[i],
            math::matrix::inverse(world.component<scene::transform>(node).to_world())));
    }

    ecs::entity joint_group = world.create("joints");
    world.add<core::link>(joint_group);
    for (auto& pmx_joint : loader.joints())
    {
        ecs::entity joint_entity = world.create("joint");
        world.add<core::link, physics::joint>(joint_entity);
        auto& joint = world.component<physics::joint>(joint_entity);

        joint.min_linear(pmx_joint.translate_min);
        joint.max_linear(pmx_joint.translate_max);
        joint.min_angular(pmx_joint.rotate_min);
        joint.max_angular(pmx_joint.rotate_max);

        joint.stiffness(0, pmx_joint.spring_translate_factor[0]);
        joint.stiffness(1, pmx_joint.spring_translate_factor[1]);
        joint.stiffness(2, pmx_joint.spring_translate_factor[2]);
        joint.stiffness(3, pmx_joint.spring_rotate_factor[0]);
        joint.stiffness(4, pmx_joint.spring_rotate_factor[1]);
        joint.stiffness(5, pmx_joint.spring_rotate_factor[2]);

        joint.spring_enable(0, pmx_joint.spring_translate_factor[0] != 0.0f);
        joint.spring_enable(1, pmx_joint.spring_translate_factor[1] != 0.0f);
        joint.spring_enable(2, pmx_joint.spring_translate_factor[2] != 0.0f);
        joint.spring_enable(3, pmx_joint.spring_rotate_factor[0] != 0.0f);
        joint.spring_enable(4, pmx_joint.spring_rotate_factor[1] != 0.0f);
        joint.spring_enable(5, pmx_joint.spring_rotate_factor[2] != 0.0f);

        math::float4x4_simd joint_world = math::matrix_simd::affine_transform(
            math::simd::set(1.0f, 1.0f, 1.0f, 0.0f),
            math::simd::load(pmx_joint.rotate),
            math::simd::load(pmx_joint.translate));

        math::float4x4_simd inverse_a = math::matrix_simd::inverse_transform_no_scale(
            math::simd::load(rigidbody_transform[pmx_joint.rigidbody_a_index]));
        math::float4x4_simd offset_a = math::matrix_simd::mul(joint_world, inverse_a);

        math::float4x4_simd inverse_b = math::matrix_simd::inverse_transform_no_scale(
            math::simd::load(rigidbody_transform[pmx_joint.rigidbody_b_index]));
        math::float4x4_simd offset_b = math::matrix_simd::mul(joint_world, inverse_b);

        math::float4_simd scale_a, position_a, rotation_a;
        math::matrix_simd::decompose(offset_a, scale_a, rotation_a, position_a);
        math::float3 relative_position_a;
        math::float4 relative_rotation_a;
        math::simd::store(position_a, relative_position_a);
        math::simd::store(rotation_a, relative_rotation_a);

        math::float4_simd scale_b, position_b, rotation_b;
        math::matrix_simd::decompose(offset_b, scale_b, rotation_b, position_b);
        math::float3 relative_position_b;
        math::float4 relative_rotation_b;
        math::simd::store(position_b, relative_position_b);
        math::simd::store(rotation_b, relative_rotation_b);

        joint.relative_rigidbody(
            rigidbody_nodes[pmx_joint.rigidbody_a_index],
            relative_position_a,
            relative_rotation_a,
            rigidbody_nodes[pmx_joint.rigidbody_b_index],
            relative_position_b,
            relative_rotation_b);

        relation.link(joint_entity, joint_group);
    }
    relation.link(joint_group, entity);
}

void mmd_loader::load_morph(
    ecs::entity entity,
    const pmx_loader& pmx_loader,
    const vmd_loader& vmd_loader)
{
    if (pmx_loader.morph().empty() || vmd_loader.morphs().empty())
        return;

    auto& world = system<ecs::world>();
    world.add<mmd_morph_controler>(entity);

    auto& morph_controler = world.component<mmd_morph_controler>(entity);
    std::map<std::string, std::size_t> morph_map;
    for (auto& pmx_morph : pmx_loader.morph())
    {
        switch (pmx_morph.type)
        {
        case pmx_morph_type::GROUP: {
            auto morph = std::make_unique<mmd_morph_controler::group_morph>();
            for (auto& pmx_group_morph : pmx_morph.group_morphs)
                morph->data.push_back({pmx_group_morph.index, pmx_group_morph.weight});

            morph_map[pmx_morph.name_jp] = morph_controler.morphs.size();
            morph_controler.morphs.push_back(std::move(morph));
            break;
        }
        case pmx_morph_type::VERTEX: {
            auto morph = std::make_unique<mmd_morph_controler::vertex_morph>();
            for (auto& pmx_vertex_morph : pmx_morph.vertex_morphs)
                morph->data.push_back({pmx_vertex_morph.index, pmx_vertex_morph.translation});

            morph_map[pmx_morph.name_jp] = morph_controler.morphs.size();
            morph_controler.morphs.push_back(std::move(morph));
            break;
        }
        case pmx_morph_type::BONE: {
            auto morph = std::make_unique<mmd_morph_controler::bone_morph>();
            for (auto& pmx_bone_morph : pmx_morph.bone_morphs)
            {
                morph->data.push_back(
                    {pmx_bone_morph.index, pmx_bone_morph.translation, pmx_bone_morph.rotation});
            }

            morph_map[pmx_morph.name_jp] = morph_controler.morphs.size();
            morph_controler.morphs.push_back(std::move(morph));
            break;
        }
        case pmx_morph_type::UV:
        case pmx_morph_type::UV_EXT_1:
        case pmx_morph_type::UV_EXT_2:
        case pmx_morph_type::UV_EXT_3:
        case pmx_morph_type::UV_EXT_4: {
            auto morph = std::make_unique<mmd_morph_controler::uv_morph>();
            morph_map[pmx_morph.name_jp] = morph_controler.morphs.size();
            morph_controler.morphs.push_back(std::move(morph));
            break;
        }
        case pmx_morph_type::MATERIAL: {
            auto morph = std::make_unique<mmd_morph_controler::material_morph>();
            morph_map[pmx_morph.name_jp] = morph_controler.morphs.size();
            morph_controler.morphs.push_back(std::move(morph));
            break;
        }
        case pmx_morph_type::FLIP: {
            auto morph = std::make_unique<mmd_morph_controler::flip_morph>();
            morph_map[pmx_morph.name_jp] = morph_controler.morphs.size();
            morph_controler.morphs.push_back(std::move(morph));
            break;
        }
        case pmx_morph_type::IMPULSE: {
            auto morph = std::make_unique<mmd_morph_controler::impulse_morph>();
            morph_map[pmx_morph.name_jp] = morph_controler.morphs.size();
            morph_controler.morphs.push_back(std::move(morph));
            break;
        }
        default:
            break;
        }
    }

    for (auto& vmd_morph : vmd_loader.morphs())
    {
        auto iter = morph_map.find(vmd_morph.morph_name);
        if (iter == morph_map.end())
            continue;

        mmd_morph_controler::key key = {};
        key.frame = vmd_morph.frame;
        key.weight = vmd_morph.weight;
        morph_controler.morphs[iter->second]->keys.push_back(key);
    }

    for (auto& morph : morph_controler.morphs)
    {
        std::sort(morph->keys.begin(), morph->keys.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
        });
    }

    morph_controler.vertex_morph_result = graphics::rhi::make_vertex_buffer<math::float3>(
        nullptr,
        pmx_loader.vertex_count(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN,
        true,
        true);
    morph_controler.uv_morph_result = graphics::rhi::make_vertex_buffer<math::float2>(
        nullptr,
        pmx_loader.vertex_count(),
        graphics::VERTEX_BUFFER_FLAG_COMPUTE_IN,
        true,
        true);

    auto& skinned_mesh = world.component<graphics::skinned_mesh>(entity);
    auto skin_parameter = static_cast<skinning_pipeline_parameter*>(skinned_mesh.parameter.get());
    skin_parameter->vertex_morph(morph_controler.vertex_morph_result.get());
    skin_parameter->uv_morph(morph_controler.uv_morph_result.get());
}

void mmd_loader::load_animation(
    ecs::entity entity,
    const pmx_loader& pmx_loader,
    const vmd_loader& vmd_loader)
{
    auto& world = system<ecs::world>();
    auto& skeletion = world.component<mmd_skeleton>(entity);

    // Node.
    std::map<std::string, mmd_node_animation*> node_map;
    for (auto& node_entity : skeletion.nodes)
    {
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
    std::map<std::string, mmd_ik_solver*> ik_map;
    for (auto& node_entity : skeletion.nodes)
    {
        if (world.has_component<mmd_ik_solver>(node_entity))
        {
            auto& bone = world.component<mmd_node>(node_entity);
            auto& ik_solver = world.component<mmd_ik_solver>(node_entity);
            ik_map[bone.name] = &ik_solver;
        }
    }

    for (auto& ik : vmd_loader.iks())
    {
        for (auto& info : ik.infos)
        {
            auto iter = ik_map.find(info.name);
            if (iter != ik_map.end())
            {
                mmd_ik_solver::key key = {};
                key.frame = ik.frame;
                key.enable = info.enable;
                iter->second->keys.push_back(key);
            }
        }
    }

    for (auto [key, value] : ik_map)
    {
        std::sort(value->keys.begin(), value->keys.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
        });
    }
}
} // namespace violet::sample::mmd
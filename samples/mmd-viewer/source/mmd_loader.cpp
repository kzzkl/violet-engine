#include "mmd_loader.hpp"
#include "components/mesh.hpp"
#include "components/mmd_animator.hpp"
#include "components/mmd_morph.hpp"
#include "components/mmd_skeleton.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "mmd_render.hpp"
#include "pmx.hpp"
#include "vmd.hpp"

namespace violet::sample
{
class rigidbody_merge_reflector : public rigidbody_reflector
{
public:
    virtual float4x4 reflect(const float4x4& rigidbody_world, const float4x4& transform_world)
        override
    {
        float4x4 result = rigidbody_world;
        result[3] = transform_world[3];
        return result;
    }
};

mmd_loader::mmd_loader(
    render_graph* render_graph,
    renderer* renderer,
    physics_context* physics_context)
    : m_render_graph(render_graph),
      m_renderer(renderer),
      m_physics_context(physics_context)
{
    std::vector<std::string> internal_toon_paths = {
        "mmd-viewer/mmd/toon01.dds",
        "mmd-viewer/mmd/toon02.dds",
        "mmd-viewer/mmd/toon03.dds",
        "mmd-viewer/mmd/toon04.dds",
        "mmd-viewer/mmd/toon05.dds",
        "mmd-viewer/mmd/toon06.dds",
        "mmd-viewer/mmd/toon07.dds",
        "mmd-viewer/mmd/toon08.dds",
        "mmd-viewer/mmd/toon09.dds",
        "mmd-viewer/mmd/toon10.dds"};
    for (const std::string& toon : internal_toon_paths)
        m_internal_toons.push_back(renderer->create_texture(toon.c_str()));

    rhi_sampler_desc sampler_desc = {};
    sampler_desc.min_filter = RHI_FILTER_LINEAR;
    sampler_desc.mag_filter = RHI_FILTER_LINEAR;
    sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    m_sampler = renderer->create_sampler(sampler_desc);
}

mmd_loader::~mmd_loader()
{
}

mmd_model* mmd_loader::load(std::string_view pmx_path, std::string_view vmd_path, world& world)
{
    if (m_models.find(pmx_path.data()) != m_models.end())
        return nullptr;

    auto model = std::make_unique<mmd_model>();

    pmx pmx(pmx_path);
    if (!pmx.is_load())
        return nullptr;

    model->model = std::make_unique<actor>("mmd", world);
    model->model->add<transform, mesh, mmd_skeleton, mmd_morph>();

    load_mesh(model.get(), pmx, world);
    load_bones(model.get(), pmx, world);
    load_morph(model.get(), pmx);
    load_physics(model.get(), pmx, world);

    if (!vmd_path.empty())
    {
        vmd vmd(vmd_path);
        if (vmd.is_load())
            load_animation(model.get(), vmd, world);
    }

    m_models[pmx_path.data()] = std::move(model);
    return m_models[pmx_path.data()].get();
}

void mmd_loader::load_mesh(mmd_model* model, const pmx& pmx, world& world)
{
    model->geometry = std::make_unique<geometry>(m_renderer);
    model->geometry->add_attribute(
        "position",
        pmx.position,
        RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_STORAGE);
    model->geometry->add_attribute(
        "normal",
        pmx.normal,
        RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_STORAGE);
    model->geometry->add_attribute("uv", pmx.uv, RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_STORAGE);
    model->geometry->add_attribute(
        "skinned position",
        pmx.position,
        RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_STORAGE);
    model->geometry->add_attribute(
        "skinned normal",
        pmx.normal,
        RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_STORAGE);
    model->geometry->add_attribute(
        "skinned uv",
        pmx.uv,
        RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_STORAGE);
    model->geometry->add_attribute("skinning type", pmx.skin, RHI_BUFFER_FLAG_STORAGE);
    model->geometry->add_attribute<float3>(
        "vertex morph",
        pmx.position.size(),
        RHI_BUFFER_FLAG_HOST_VISIBLE | RHI_BUFFER_FLAG_STORAGE);
    model->geometry->set_indices(pmx.indices);

    for (const std::string& texture : pmx.textures)
    {
        try
        {
            model->textures.push_back(
                m_renderer->create_texture(texture.c_str(), RHI_TEXTURE_FLAG_MIPMAP));
        }
        catch (...)
        {
            model->textures.push_back(nullptr);
        }
    }
    for (const pmx_material& pmx_material : pmx.materials)
    {
        material* material = m_render_graph->add_material(pmx_material.name_jp, "mmd material");

        material->set(
            "mmd material",
            mmd_material{
                .diffuse = pmx_material.diffuse,
                .specular = pmx_material.specular,
                .specular_strength = pmx_material.specular_strength,
                .edge_color = pmx_material.edge_color,
                .ambient = pmx_material.ambient,
                .edge_size = pmx_material.edge_size,
                .toon_mode = pmx_material.toon_mode,
                .spa_mode = pmx_material.sphere_mode});

        material->set(
            "mmd tex",
            model->textures[pmx_material.texture_index].get(),
            m_sampler.get());
        if (pmx_material.toon_index != -1)
        {
            if (pmx_material.toon_mode == PMX_TOON_MODE_TEXTURE)
                material->set(
                    "mmd toon",
                    model->textures[pmx_material.toon_index].get(),
                    m_sampler.get());
            else
                material->set(
                    "mmd toon",
                    m_internal_toons[pmx_material.toon_index].get(),
                    m_sampler.get());
        }
        else
        {
            material->set("mmd toon", m_internal_toons[0].get(), m_sampler.get());
        }

        if (pmx_material.sphere_mode != PMX_SPHERE_MODE_DISABLED)
            material->set(
                "mmd spa",
                model->textures[pmx_material.sphere_index].get(),
                m_sampler.get());
        else
            material->set("mmd spa", m_internal_toons[0].get(), m_sampler.get());

        model->materials.push_back(material);
    }

    auto model_mesh = model->model->get<mesh>();
    model_mesh->set_geometry(model->geometry.get());
    for (auto& submesh : pmx.submeshes)
    {
        model_mesh->add_submesh(
            0,
            pmx.position.size(),
            submesh.index_start,
            submesh.index_count,
            model->materials[submesh.material_index]);
    }
}

void mmd_loader::load_bones(mmd_model* model, const pmx& pmx, world& world)
{
    auto model_skeleton = model->model->get<mmd_skeleton>();
    model_skeleton->bones.resize(pmx.bones.size());

    model->bones.reserve(pmx.bones.size());
    for (std::size_t i = 0; i < pmx.bones.size(); ++i)
    {
        auto bone = std::make_unique<actor>(pmx.bones[i].name_jp, world);
        auto [bone_transform] = bone->add<transform>();

        model->bones.push_back(std::move(bone));
        model_skeleton->bones[i].transform = bone_transform;
        model_skeleton->bones[i].index = static_cast<std::uint32_t>(i);
    }

    for (std::size_t i = 0; i < pmx.bones.size(); ++i)
    {
        auto& pmx_bone = pmx.bones[i];
        auto bone_transform = model->bones[i]->get<transform>();

        if (pmx_bone.parent_index != -1)
        {
            bone_transform->set_position(
                vector::sub(pmx_bone.position, pmx.bones[pmx_bone.parent_index].position));
            auto& parent_bone = model->bones[pmx_bone.parent_index];
            parent_bone->get<transform>()->add_child(bone_transform);
        }
        else
        {
            bone_transform->set_position(pmx_bone.position);
            model->model->get<transform>()->add_child(bone_transform);
        }
    }

    for (std::size_t i = 0; i < pmx.bones.size(); ++i)
    {
        auto& pmx_bone = pmx.bones[i];
        auto& bone_info = model_skeleton->bones[i];
        auto bone_transform = model->bones[i]->get<transform>();

        float4x4_simd initial = simd::load(bone_transform->get_world_matrix());
        float4x4_simd inverse = matrix_simd::inverse_transform_no_scale(initial);
        simd::store(inverse, bone_info.initial_inverse);

        bone_info.layer = pmx_bone.layer;
        bone_info.deform_after_physics = pmx_bone.flags & PMX_BONE_FLAG_PHYSICS_AFTER_DEFORM;
        bone_info.is_inherit_rotation = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_ROTATION;
        bone_info.is_inherit_translation = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_TRANSLATION;
        bone_info.inherit_index = pmx_bone.inherit_index;

        if ((bone_info.is_inherit_rotation || bone_info.is_inherit_translation) &&
            pmx_bone.inherit_index != -1)
        {
            bone_info.inherit_local_flag = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_LOCAL;
            bone_info.inherit_weight = pmx_bone.inherit_weight;
        }

        bone_info.initial_position = bone_transform->get_position();
        bone_info.initial_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        bone_info.initial_scale = {1.0f, 1.0f, 1.0f};
    }

    for (std::size_t i = 0; i < pmx.bones.size(); ++i)
    {
        auto& pmx_bone = pmx.bones[i];
        if (pmx_bone.flags & PMX_BONE_FLAG_IK)
        {
            auto& bone_info = model_skeleton->bones[i];
            bone_info.ik_solver = std::make_unique<mmd_skeleton::bone_ik_solver>();
            bone_info.ik_solver->enable = true;
            bone_info.ik_solver->iteration_count = pmx_bone.ik_iteration_count;
            bone_info.ik_solver->limit = pmx_bone.ik_limit;
            bone_info.ik_solver->target_index = pmx_bone.ik_target_index;

            for (auto& pmx_ik_link : pmx_bone.ik_links)
            {
                auto& link_bone = model_skeleton->bones[pmx_ik_link.bone_index];
                link_bone.ik_link = std::make_unique<mmd_skeleton::bone_ik_link>();
                if (pmx_ik_link.enable_limit)
                {
                    link_bone.ik_link->enable_limit = true;
                    link_bone.ik_link->limit_min = pmx_ik_link.limit_min;
                    link_bone.ik_link->limit_max = pmx_ik_link.limit_max;
                    link_bone.ik_link->save_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
                }
                else
                {
                    link_bone.ik_link->enable_limit = false;
                    link_bone.ik_link->save_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
                }

                bone_info.ik_solver->links.push_back(pmx_ik_link.bone_index);
            }
        }
    }

    model_skeleton->sorted_bones.resize(model_skeleton->bones.size());
    for (std::size_t i = 0; i < model_skeleton->sorted_bones.size(); ++i)
        model_skeleton->sorted_bones[i] = i;
    std::stable_sort(
        model_skeleton->sorted_bones.begin(),
        model_skeleton->sorted_bones.end(),
        [&](std::size_t a, std::size_t b) -> bool
        {
            return model_skeleton->bones[a].layer < model_skeleton->bones[b].layer;
        });

    model_skeleton->local_matrices.resize(model->bones.size());
    model_skeleton->world_matrices.resize(model->bones.size());

    model_skeleton->set_skinning_data(pmx.bdef, pmx.sdef);
    model_skeleton->set_skinning_input(
        model->geometry->get_vertex_buffer("position"),
        model->geometry->get_vertex_buffer("normal"),
        model->geometry->get_vertex_buffer("uv"),
        model->geometry->get_vertex_buffer("skinning type"),
        model->geometry->get_vertex_buffer("vertex morph"));
    model_skeleton->set_skinning_output(
        model->geometry->get_vertex_buffer("skinned position"),
        model->geometry->get_vertex_buffer("skinned normal"),
        model->geometry->get_vertex_buffer("skinned uv"));
}

void mmd_loader::load_morph(mmd_model* model, const pmx& pmx)
{
    auto model_morph = model->model->get<mmd_morph>();
    model_morph->vertex_morph_result = model->geometry->get_vertex_buffer("vertex morph");

    for (auto& pmx_morph : pmx.morphs)
    {
        switch (pmx_morph.type)
        {
        case PMX_MORPH_TYPE_GROUP:
            model_morph->morphs.push_back(std::make_unique<mmd_morph::morph>());
            break;
        case PMX_MORPH_TYPE_VERTEX: {
            auto vertex_morph = std::make_unique<mmd_morph::vertex_morph>();
            for (auto& pmx_vertex_morph : pmx_morph.vertex_morphs)
            {
                vertex_morph->data.push_back(
                    {pmx_vertex_morph.index, pmx_vertex_morph.translation});
            }
            model_morph->morphs.push_back(std::move(vertex_morph));
            break;
        }
        case PMX_MORPH_TYPE_BONE:
        case PMX_MORPH_TYPE_UV:
        case PMX_MORPH_TYPE_UV_EXT_1:
        case PMX_MORPH_TYPE_UV_EXT_2:
        case PMX_MORPH_TYPE_UV_EXT_3:
        case PMX_MORPH_TYPE_UV_EXT_4:
        case PMX_MORPH_TYPE_MATERIAL:
        case PMX_MORPH_TYPE_FLIP:
        case PMX_MORPH_TYPE_IMPULSE:
            model_morph->morphs.push_back(std::make_unique<mmd_morph::morph>());
            break;
        default:
            break;
        }

        model_morph->morphs.back()->name = pmx_morph.name_jp;
    }
}

void mmd_loader::load_physics(mmd_model* model, const pmx& pmx, world& world)
{
    std::vector<std::size_t> rigidbody_count(pmx.bones.size());
    std::vector<float4x4> rigidbody_transform;
    rigidbody_transform.reserve(pmx.rigidbodies.size());
    for (auto& pmx_rigidbody : pmx.rigidbodies)
    {
        rigidbody_transform.push_back(matrix::affine_transform(
            float3{1.0f, 1.0f, 1.0f},
            pmx_rigidbody.rotate,
            pmx_rigidbody.translate));
        ++rigidbody_count[pmx_rigidbody.bone_index];
    }

    std::vector<actor*> rigidbody_bones;
    rigidbody_bones.reserve(pmx.rigidbodies.size());
    for (std::size_t i = 0; i < pmx.rigidbodies.size(); ++i)
    {
        auto& pmx_rigidbody = pmx.rigidbodies[i];

        actor* bone = model->bones[pmx_rigidbody.bone_index].get();
        if (rigidbody_count[pmx_rigidbody.bone_index] == 1)
        {
            bone->add<rigidbody>();
        }
        else
        {
            // Workaround multiple rigid bodies are attached to a node.
            auto workaround_actor = std::make_unique<actor>("rigidbody", world);
            workaround_actor->add<rigidbody, transform>();
            bone->get<transform>()->add_child(workaround_actor->get<transform>());
            auto matrix = workaround_actor->get<transform>()->get_world_matrix();
            bone = workaround_actor.get();

            model->bones.push_back(std::move(workaround_actor));
        }
        rigidbody_bones.push_back(bone);

        pei_collision_shape_desc shape_desc = {};
        switch (pmx_rigidbody.shape)
        {
        case PMX_RIGIDBODY_SHAPE_TYPE_SPHERE:
            shape_desc.type = PEI_COLLISION_SHAPE_TYPE_SPHERE;
            shape_desc.sphere.radius = pmx_rigidbody.size[0];
            break;
        case PMX_RIGIDBODY_SHAPE_TYPE_BOX:
            shape_desc.type = PEI_COLLISION_SHAPE_TYPE_BOX;
            shape_desc.box.length = pmx_rigidbody.size[0] * 2.0f;
            shape_desc.box.height = pmx_rigidbody.size[1] * 2.0f;
            shape_desc.box.width = pmx_rigidbody.size[2] * 2.0f;
            break;
        case PMX_RIGIDBODY_SHAPE_TYPE_CAPSULE:
            shape_desc.type = PEI_COLLISION_SHAPE_TYPE_CAPSULE;
            shape_desc.capsule.radius = pmx_rigidbody.size[0];
            shape_desc.capsule.height = pmx_rigidbody.size[1];
            break;
        default:
            break;
        };
        model->collision_shapes.push_back(m_physics_context->create_collision_shape(shape_desc));

        auto bone_rigidbody = bone->get<rigidbody>();
        bone_rigidbody->set_activation_state(PEI_RIGIDBODY_ACTIVATION_STATE_DISABLE_DEACTIVATION);
        bone_rigidbody->set_shape(model->collision_shapes[i].get());

        switch (pmx_rigidbody.mode)
        {
        case PMX_RIGIDBODY_MODE_STATIC:
            bone_rigidbody->set_type(PEI_RIGIDBODY_TYPE_KINEMATIC);
            break;
        case PMX_RIGIDBODY_MODE_DYNAMIC:
            bone_rigidbody->set_type(PEI_RIGIDBODY_TYPE_DYNAMIC);
            break;
        case PMX_RIGIDBODY_MODE_MERGE:
            bone_rigidbody->set_type(PEI_RIGIDBODY_TYPE_DYNAMIC);
            bone_rigidbody->set_reflector<rigidbody_merge_reflector>();
            break;
        default:
            break;
        }

        bone_rigidbody->set_mass(
            pmx_rigidbody.mode == PMX_RIGIDBODY_MODE_STATIC ? 0.0f : pmx_rigidbody.mass);
        bone_rigidbody->set_damping(pmx_rigidbody.linear_damping, pmx_rigidbody.angular_damping);
        bone_rigidbody->set_restitution(pmx_rigidbody.repulsion);
        bone_rigidbody->set_friction(pmx_rigidbody.friction);
        bone_rigidbody->set_collision_group(static_cast<std::size_t>(1) << pmx_rigidbody.group);
        bone_rigidbody->set_collision_mask(pmx_rigidbody.collision_group);
        bone_rigidbody->set_offset(matrix::mul(
            rigidbody_transform[i],
            matrix::inverse(bone->get<transform>()->get_world_matrix())));
        bone_rigidbody->set_transform(rigidbody_transform[i]);
    }

    for (auto& pmx_joint : pmx.joints)
    {
        auto rigidbody_a = rigidbody_bones[pmx_joint.rigidbody_a_index]->get<rigidbody>();
        auto rigidbody_b = rigidbody_bones[pmx_joint.rigidbody_b_index]->get<rigidbody>();

        float4x4_simd joint_world = matrix_simd::affine_transform(
            simd::set(1.0f, 1.0f, 1.0f, 0.0f),
            simd::load(pmx_joint.rotate),
            simd::load(pmx_joint.translate));

        float4x4_simd inverse_a = matrix_simd::inverse_transform_no_scale(
            simd::load(rigidbody_transform[pmx_joint.rigidbody_a_index]));
        float4x4_simd offset_a = matrix_simd::mul(joint_world, inverse_a);

        float4_simd scale_a, position_a, rotation_a;
        matrix_simd::decompose(offset_a, scale_a, rotation_a, position_a);
        float3 relative_position_a;
        float4 relative_rotation_a;
        simd::store(position_a, relative_position_a);
        simd::store(rotation_a, relative_rotation_a);

        float4x4_simd inverse_b = matrix_simd::inverse_transform_no_scale(
            simd::load(rigidbody_transform[pmx_joint.rigidbody_b_index]));
        float4x4_simd offset_b = matrix_simd::mul(joint_world, inverse_b);

        float4_simd scale_b, position_b, rotation_b;
        matrix_simd::decompose(offset_b, scale_b, rotation_b, position_b);
        float3 relative_position_b;
        float4 relative_rotation_b;
        simd::store(position_b, relative_position_b);
        simd::store(rotation_b, relative_rotation_b);

        joint* joint = rigidbody_a->add_joint(
            rigidbody_b,
            relative_position_a,
            relative_rotation_a,
            relative_position_b,
            relative_rotation_b);
        joint->set_linear(pmx_joint.translate_min, pmx_joint.translate_max);
        joint->set_angular(pmx_joint.rotate_min, pmx_joint.rotate_max);
        joint->set_stiffness(0, pmx_joint.spring_translate_factor[0]);
        joint->set_stiffness(1, pmx_joint.spring_translate_factor[1]);
        joint->set_stiffness(2, pmx_joint.spring_translate_factor[2]);
        joint->set_stiffness(3, pmx_joint.spring_rotate_factor[0]);
        joint->set_stiffness(4, pmx_joint.spring_rotate_factor[1]);
        joint->set_stiffness(5, pmx_joint.spring_rotate_factor[2]);
        joint->set_spring_enable(0, pmx_joint.spring_translate_factor[0] != 0.0f);
        joint->set_spring_enable(1, pmx_joint.spring_translate_factor[1] != 0.0f);
        joint->set_spring_enable(2, pmx_joint.spring_translate_factor[2] != 0.0f);
        joint->set_spring_enable(3, pmx_joint.spring_rotate_factor[0] != 0.0f);
        joint->set_spring_enable(4, pmx_joint.spring_rotate_factor[1] != 0.0f);
        joint->set_spring_enable(5, pmx_joint.spring_rotate_factor[2] != 0.0f);
    }
}

void mmd_loader::load_animation(mmd_model* model, const vmd& vmd, world& world)
{
    auto model_skeleton = model->model->get<mmd_skeleton>();
    auto [model_animator] = model->model->add<mmd_animator>();
    model_animator->motions.resize(model_skeleton->bones.size());

    std::map<std::string, std::size_t> motion_map;
    for (std::size_t i = 0; i < model_skeleton->bones.size(); ++i)
    {
        std::string name = model_skeleton->bones[i].transform.get_owner()->get_name();
        motion_map[name] = i;
    }

    auto set_bezier = [](bezier& bezier, const unsigned char* cp)
    {
        int x0 = cp[0];
        int y0 = cp[4];
        int x1 = cp[8];
        int y1 = cp[12];

        bezier.set(
            {static_cast<float>(x0) / 127.0f, static_cast<float>(y0) / 127.0f},
            {static_cast<float>(x1) / 127.0f, static_cast<float>(y1) / 127.0f});
    };

    for (auto& vmd_motion : vmd.motions)
    {
        auto iter = motion_map.find(vmd_motion.bone_name);
        if (iter != motion_map.end())
        {
            mmd_animator::animation_key key;
            key.frame = vmd_motion.frame_index;
            key.translate = vmd_motion.translate;
            key.rotate = vmd_motion.rotate;

            set_bezier(key.tx_bezier, &vmd_motion.interpolation[0]);
            set_bezier(key.ty_bezier, &vmd_motion.interpolation[1]);
            set_bezier(key.tz_bezier, &vmd_motion.interpolation[2]);
            set_bezier(key.r_bezier, &vmd_motion.interpolation[3]);

            model_animator->motions[iter->second].animation_keys.push_back(key);
        }
    }

    for (auto& motion : model_animator->motions)
    {
        std::sort(
            motion.animation_keys.begin(),
            motion.animation_keys.end(),
            [](const mmd_animator::animation_key& a, const mmd_animator::animation_key& b)
            {
                return a.frame < b.frame;
            });
    }

    std::map<std::string, std::size_t> ik_map;
    for (std::size_t i = 0; i < model_skeleton->bones.size(); ++i)
        ik_map[model->bones[i]->get_name()] = i;

    for (auto& ik : vmd.iks)
    {
        for (auto& info : ik.infos)
        {
            auto iter = ik_map.find(info.name);
            if (iter != ik_map.end())
            {
                mmd_animator::ik_key key = {};
                key.frame = ik.frame;
                key.enable = info.enable;
                model_animator->motions[iter->second].ik_keys.push_back(key);
            }
        }
    }

    for (auto& motion : model_animator->motions)
    {
        std::sort(
            motion.ik_keys.begin(),
            motion.ik_keys.end(),
            [](const auto& a, const auto& b)
            {
                return a.frame < b.frame;
            });
    }

    auto model_morph = model->model->get<mmd_morph>();
    model_animator->morphs.resize(model_morph->morphs.size());

    std::map<std::string, std::size_t> morph_map;
    for (std::size_t i = 0; i < model_morph->morphs.size(); ++i)
        morph_map[model_morph->morphs[i]->name] = i;

    for (auto& morph : vmd.morphs)
    {
        auto iter = morph_map.find(morph.morph_name);
        if (iter != morph_map.end())
        {
            mmd_animator::morph_key key = {};
            key.frame = morph.frame;
            key.weight = morph.weight;
            model_animator->morphs[iter->second].morph_keys.push_back(key);
        }
    }

    for (auto& morph : model_animator->morphs)
    {
        std::sort(
            morph.morph_keys.begin(),
            morph.morph_keys.end(),
            [](const auto& a, const auto& b)
            {
                return a.frame < b.frame;
            });
    }
}
} // namespace violet::sample
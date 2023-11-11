#include "mmd_loader.hpp"
#include "components/mesh.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "mmd_render.hpp"
#include "mmd_skeleton.hpp"
#include "pmx_loader.hpp"

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

mmd_loader::mmd_loader(render_graph* render_graph, rhi_renderer* rhi, pei_plugin* pei)
    : m_render_graph(render_graph),
      m_rhi(rhi),
      m_pei(pei)
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
        m_internal_toons.push_back(rhi->create_texture(toon.c_str()));

    rhi_sampler_desc sampler_desc = {};
    sampler_desc.min_filter = RHI_FILTER_LINEAR;
    sampler_desc.mag_filter = RHI_FILTER_LINEAR;
    sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    m_sampler = rhi->create_sampler(sampler_desc);
}

mmd_loader::~mmd_loader()
{
    for (auto& [name, model] : m_models)
    {
        for (rhi_resource* texture : model->textures)
            m_rhi->destroy_texture(texture);
    }

    m_models.clear();

    for (rhi_resource* toon : m_internal_toons)
        m_rhi->destroy_texture(toon);

    m_rhi->destroy_sampler(m_sampler);
}

mmd_model* mmd_loader::load(std::string_view path, world& world)
{
    pmx_loader loader;
    if (m_models.find(path.data()) != m_models.end() || !loader.load(path))
        return nullptr;

    const pmx_mesh& pmx = loader.get_mesh();
    auto model = std::make_unique<mmd_model>();

    model->geometry = std::make_unique<geometry>(m_rhi);
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
    model->geometry->set_indices(pmx.indices);

    for (const std::string& texture : pmx.textures)
        model->textures.push_back(m_rhi->create_texture(texture.c_str()));
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

        material->set("mmd tex", model->textures[pmx_material.texture_index], m_sampler);
        if (pmx_material.toon_mode == PMX_TOON_MODE_TEXTURE)
            material->set("mmd toon", model->textures[pmx_material.toon_index], m_sampler);
        else
            material->set("mmd toon", m_internal_toons[pmx_material.toon_index], m_sampler);

        if (pmx_material.sphere_mode != PMX_SPHERE_MODE_DISABLED)
            material->set("mmd spa", model->textures[pmx_material.sphere_index], m_sampler);
        else
            material->set("mmd spa", m_internal_toons[0], m_sampler);

        model->materials.push_back(material);
    }

    model->model = std::make_unique<actor>("mmd", world);
    auto [transform_ptr, mesh_ptr, skeleton_ptr] =
        model->model->add<transform, mesh, mmd_skeleton>();
    mesh_ptr->set_geometry(model->geometry.get());

    for (auto& submesh : pmx.submeshes)
    {
        mesh_ptr->add_submesh(
            0,
            pmx.position.size(),
            submesh.index_start,
            submesh.index_count,
            model->materials[submesh.material_index]);
    }

    load_bones(model.get(), pmx, world);
    load_physics(model.get(), pmx, world);

    m_models[path.data()] = std::move(model);
    return m_models[path.data()].get();
}

void mmd_loader::load_bones(mmd_model* model, const pmx_mesh& pmx, world& world)
{
    model->bones.reserve(pmx.bones.size());
    for (std::size_t i = 0; i < pmx.bones.size(); ++i)
    {
        auto bone = std::make_unique<actor>(pmx.bones[i].name_jp, world);
        auto [bone_transform, bone_info] = bone->add<transform, mmd_bone>();
        bone_info->index = static_cast<std::uint32_t>(i);

        model->bones.push_back(std::move(bone));
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
        auto bone_info = model->bones[i]->get<mmd_bone>();
        auto bone_transform = model->bones[i]->get<transform>();

        float4x4_simd initial = simd::load(bone_transform->get_world_matrix());
        float4x4_simd inverse = matrix_simd::inverse_transform_no_scale(initial);
        simd::store(inverse, bone_info->initial_inverse);

        bone_info->layer = pmx_bone.layer;
        bone_info->deform_after_physics = pmx_bone.flags & PMX_BONE_FLAG_PHYSICS_AFTER_DEFORM;
        bone_info->is_inherit_rotation = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_ROTATION;
        bone_info->is_inherit_translation = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_TRANSLATION;

        if ((bone_info->is_inherit_rotation || bone_info->is_inherit_translation) &&
            pmx_bone.inherit_index != -1)
        {
            bone_info->inherit_local_flag = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_LOCAL;
            bone_info->inherit_node = model->bones[pmx_bone.inherit_index].get();
            bone_info->inherit_weight = pmx_bone.inherit_weight;
        }

        bone_info->initial_position = bone_transform->get_position();
        bone_info->initial_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        bone_info->initial_scale = {1.0f, 1.0f, 1.0f};
    }

    auto model_skeleton = model->model->get<mmd_skeleton>();
    model_skeleton->bones.resize(model->bones.size());
    std::transform(
        model->bones.begin(),
        model->bones.end(),
        model_skeleton->bones.begin(),
        [](auto& bone)
        {
            return bone.get();
        });
    model_skeleton->sorted_bones = model_skeleton->bones;
    std::stable_sort(
        model_skeleton->sorted_bones.begin(),
        model_skeleton->sorted_bones.end(),
        [&](actor* a, actor* b) -> bool
        {
            return a->get<mmd_bone>()->layer < b->get<mmd_bone>()->layer;
        });
    model_skeleton->local_matrices.resize(model->bones.size());
    model_skeleton->world_matrices.resize(model->bones.size());

    model_skeleton->set_skinning_data(pmx.bdef, pmx.sdef);
    model_skeleton->set_skinning_input(
        model->geometry->get_vertex_buffer("position"),
        model->geometry->get_vertex_buffer("normal"),
        model->geometry->get_vertex_buffer("uv"),
        model->geometry->get_vertex_buffer("skinning type"));
    model_skeleton->set_skinning_output(
        model->geometry->get_vertex_buffer("skinned position"),
        model->geometry->get_vertex_buffer("skinned normal"),
        model->geometry->get_vertex_buffer("skinned uv"));
}

void mmd_loader::load_physics(mmd_model* model, const pmx_mesh& pmx, world& world)
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
        model->collision_shapes.push_back(m_pei->create_collision_shape(shape_desc));

        auto bone_rigidbody = bone->get<rigidbody>();
        bone_rigidbody->set_shape(model->collision_shapes[i]);

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
        bone_rigidbody->set_collision_group(1 << pmx_rigidbody.group);
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

        joint* joint = rigidbody_a->add_joint(relative_position_a, relative_rotation_a);
        joint->set_target(rigidbody_b, relative_position_b, relative_rotation_b);
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
} // namespace violet::sample
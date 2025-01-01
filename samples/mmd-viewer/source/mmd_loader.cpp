#include "mmd_loader.hpp"
#include "components/collider_component.hpp"
#include "components/hierarchy_component.hpp"
#include "components/joint_component.hpp"
#include "components/mesh_component.hpp"
#include "components/mmd_animator_component.hpp"
#include "components/mmd_skeleton_component.hpp"
#include "components/morph_component.hpp"
#include "components/rigidbody_component.hpp"
#include "components/scene_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/skinned_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/tools/texture_loader.hpp"
#include "math/matrix.hpp"
#include "math/vector.hpp"
#include "mmd_material.hpp"
#include "vmd.hpp"
#include <map>
#include <numeric>

namespace violet
{
struct mmd_skinning_cs : public skinning_cs
{
    static constexpr std::string_view path = "assets/shaders/mmd_skinning.hlsl";
};

mmd_loader::mmd_loader(const std::vector<rhi_texture*>& internal_toons)
    : m_internal_toons(internal_toons)
{
}

mmd_loader::~mmd_loader() {}

std::optional<mmd_loader::scene_data> mmd_loader::load(
    std::string_view pmx,
    std::string_view vmd,
    world& world)
{
    if (!m_pmx.load(pmx))
    {
        return std::nullopt;
    }

    if (!vmd.empty() && !m_vmd.load(vmd))
    {
        return std::nullopt;
    }

    m_root = world.create();

    if (vmd.empty())
    {
        world.add_component<
            transform_component,
            mesh_component,
            skinned_component,
            skeleton_component,
            mmd_skeleton_component,
            morph_component,
            scene_component>(m_root);
    }
    else
    {
        world.add_component<
            transform_component,
            mesh_component,
            skinned_component,
            skeleton_component,
            mmd_skeleton_component,
            morph_component,
            mmd_animator_component,
            scene_component>(m_root);
    }

    scene_data scene_data;
    scene_data.root = m_root;

    load_mesh(scene_data, world);
    load_bone(world);
    load_morph(world);
    load_physics(world);

    if (!vmd.empty())
    {
        load_animation(world);
    }

    return scene_data;
}

void mmd_loader::load_mesh(scene_data& scene, world& world)
{
    auto mesh_geometry = std::make_unique<geometry>();

    mesh_geometry = std::make_unique<geometry>();
    mesh_geometry->add_attribute(
        "position",
        m_pmx.position,
        RHI_BUFFER_VERTEX | RHI_BUFFER_STORAGE);
    mesh_geometry->add_attribute("normal", m_pmx.normal, RHI_BUFFER_VERTEX | RHI_BUFFER_STORAGE);
    mesh_geometry->add_attribute(
        "texcoord",
        m_pmx.texcoord,
        RHI_BUFFER_VERTEX | RHI_BUFFER_STORAGE);
    mesh_geometry->add_attribute("edge", m_pmx.edge, RHI_BUFFER_VERTEX);
    mesh_geometry->add_attribute("skin", m_pmx.skin, RHI_BUFFER_STORAGE);
    mesh_geometry->add_attribute("bdef", m_pmx.bdef, RHI_BUFFER_STORAGE);
    if (m_pmx.sdef.empty())
    {
        // Workaround for PMX files without sdef data.
        std::vector<pmx::sdef_data> empty(1);
        mesh_geometry->add_attribute("sdef", empty, RHI_BUFFER_STORAGE);
    }
    else
    {
        mesh_geometry->add_attribute("sdef", m_pmx.sdef, RHI_BUFFER_STORAGE);
    }
    mesh_geometry->set_indexes(m_pmx.indexes);
    mesh_geometry->set_vertex_count(m_pmx.position.size());
    mesh_geometry->set_index_count(m_pmx.indexes.size());

    scene.geometries.push_back(std::move(mesh_geometry));

    for (const auto& texture : m_pmx.textures)
    {
        scene.textures.push_back(texture_loader::load(
            texture,
            TEXTURE_LOAD_OPTION_GENERATE_MIPMAPS | TEXTURE_LOAD_OPTION_SRGB));
    }

    std::vector<std::pair<material*, material*>> materials;
    for (const auto& pmx_material : m_pmx.materials)
    {
        // Main material.
        auto material = std::make_unique<mmd_material>();

        material->set_diffuse(pmx_material.diffuse);
        material->set_specular(pmx_material.specular, pmx_material.specular_strength);
        material->set_ambient(pmx_material.ambient);

        material->set_diffuse(scene.textures[pmx_material.texture_index].get());
        if (pmx_material.toon_index != -1)
        {
            if (pmx_material.toon_reference == PMX_TOON_REFERENCE_TEXTURE)
            {
                material->set_toon(scene.textures[pmx_material.toon_index].get());
            }
            else
            {
                material->set_toon(m_internal_toons[pmx_material.toon_index]);
            }
        }

        if (pmx_material.environment_blend_mode != PMX_ENVIRONMENT_BLEND_MODE_DISABLED)
        {
            material->set_environment(scene.textures[pmx_material.environment_index].get());
        }

        material->set_environment_blend(pmx_material.environment_blend_mode);
        scene.materials.push_back(std::move(material));

        materials.emplace_back(scene.materials.back().get(), nullptr);

        // Outline material.
        if (pmx_material.flags & PMX_DRAW_FLAG_HAS_EDGE)
        {
            auto outline_material = std::make_unique<mmd_outline_material>();
            outline_material->set_outline(pmx_material.outline_color, pmx_material.outline_width);
            scene.materials.push_back(std::move(outline_material));

            materials.back().second = scene.materials.back().get();
        }
    }

    auto& root_mesh = world.get_component<mesh_component>(m_root);
    root_mesh.geometry = scene.geometries[0].get();
    for (const auto& submesh : m_pmx.submeshes)
    {
        auto [main_material, outline_material] = materials[submesh.material_index];

        root_mesh.submeshes.push_back({
            .vertex_offset = 0,
            .index_offset = static_cast<std::uint32_t>(submesh.index_offset),
            .index_count = static_cast<std::uint32_t>(submesh.index_count),
            .material = main_material,
        });

        if (outline_material != nullptr)
        {
            root_mesh.submeshes.push_back({
                .vertex_offset = 0,
                .index_offset = static_cast<std::uint32_t>(submesh.index_offset),
                .index_count = static_cast<std::uint32_t>(submesh.index_count),
                .material = outline_material,
            });
        }
    }

    auto& root_skinned = world.get_component<skinned_component>(m_root);
    root_skinned.inputs = {"position", "normal", "skin", "bdef", "sdef", "morph"};
    root_skinned.outputs = {
        {"position", RHI_FORMAT_R32G32B32_FLOAT},
        {"normal", RHI_FORMAT_R32G32B32_FLOAT},
    };
    root_skinned.shader = render_device::instance().get_shader<mmd_skinning_cs>();
}

void mmd_loader::load_bone(world& world)
{
    for (const auto& pmx_bone : m_pmx.bones)
    {
        entity bone = world.create();
        world.add_component<transform_component, parent_component, scene_component>(bone);

        m_bones.push_back(bone);
        m_bone_initial_transforms.push_back(matrix::translation(pmx_bone.position));
    }

    auto& mmd_skeleton = world.get_component<mmd_skeleton_component>(m_root);
    mmd_skeleton.bones.resize(m_pmx.bones.size());

    for (std::size_t i = 0; i < m_pmx.bones.size(); ++i)
    {
        const auto& pmx_bone = m_pmx.bones[i];

        entity bone_entity = m_bones[i];
        auto& bone_transform = world.get_component<transform_component>(bone_entity);
        auto& bone_parent = world.get_component<parent_component>(bone_entity);

        if (pmx_bone.parent_index != -1)
        {
            bone_parent.parent = m_bones[pmx_bone.parent_index];
            bone_transform.set_position(
                vector::sub(pmx_bone.position, m_pmx.bones[pmx_bone.parent_index].position));
        }
        else
        {
            bone_parent.parent = m_root;
            bone_transform.set_position(pmx_bone.position);
        }

        auto& bone = mmd_skeleton.bones[i];
        bone.name = pmx_bone.name_jp;
        bone.entity = bone_entity;
        bone.index = i;
        bone.update_after_physics = pmx_bone.flags & PMX_BONE_FLAG_PHYSICS_AFTER_DEFORM;
        bone.is_inherit_rotation = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_ROTATION;
        bone.is_inherit_translation = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_TRANSLATION;
        bone.inherit_index = pmx_bone.inherit_index;

        if ((bone.is_inherit_rotation || bone.is_inherit_translation) &&
            pmx_bone.inherit_index != -1)
        {
            bone.inherit_local_flag = pmx_bone.flags & PMX_BONE_FLAG_INHERIT_LOCAL;
            bone.inherit_weight = pmx_bone.inherit_weight;
        }

        bone.initial_position = bone_transform.get_position();

        if (pmx_bone.flags & PMX_BONE_FLAG_IK)
        {
            bone.ik_solver = std::make_unique<mmd_ik_solver>();
            bone.ik_solver->enable = true;
            bone.ik_solver->iteration_count = pmx_bone.ik_iteration_count;
            bone.ik_solver->limit = pmx_bone.ik_limit;
            bone.ik_solver->target = pmx_bone.ik_target_index;

            for (const auto& pmx_ik_link : pmx_bone.ik_links)
            {
                auto& link_bone = mmd_skeleton.bones[pmx_ik_link.bone_index];
                link_bone.ik_link = std::make_unique<mmd_ik_link>();
                if (pmx_ik_link.enable_limit)
                {
                    link_bone.ik_link->enable_limit = true;
                    link_bone.ik_link->limit_min = pmx_ik_link.limit_min;
                    link_bone.ik_link->limit_max = pmx_ik_link.limit_max;
                }
                else
                {
                    link_bone.ik_link->enable_limit = false;
                }

                bone.ik_solver->links.push_back(pmx_ik_link.bone_index);
            }
        }
    }

    mmd_skeleton.sorted_bones.resize(m_bones.size());
    std::iota(mmd_skeleton.sorted_bones.begin(), mmd_skeleton.sorted_bones.end(), 0);

    std::stable_sort(
        mmd_skeleton.sorted_bones.begin(),
        mmd_skeleton.sorted_bones.end(),
        [&](std::size_t a, std::size_t b) -> bool
        {
            return m_pmx.bones[a].layer < m_pmx.bones[b].layer;
        });

    auto& skeleton = world.get_component<skeleton_component>(m_root);
    for (std::size_t i = 0; i < m_pmx.bones.size(); ++i)
    {
        skeleton_bone bone = {
            .entity = m_bones[i],
        };

        mat4f_simd binding_pose_inv = matrix::inverse(math::load(m_bone_initial_transforms[i]));
        math::store(binding_pose_inv, bone.binding_pose_inv);

        skeleton.bones.push_back(bone);
    }
}

void mmd_loader::load_morph(world& world)
{
    const auto& mmd_skeleton = world.get_component<const mmd_skeleton_component>(m_root);
    const auto& mesh = world.get_component<const mesh_component>(m_root);

    auto& morph = world.get_component<morph_component>(m_root);
    morph.weights.resize(m_pmx.morphs.size());

    for (const auto& pmx_morph : m_pmx.morphs)
    {
        std::vector<morph_element> morph_elements;
        switch (pmx_morph.type)
        {
        case PMX_MORPH_TYPE_GROUP: {
            break;
        }
        case PMX_MORPH_TYPE_VERTEX: {
            for (const auto& pmx_vertex_morph : pmx_morph.vertex_morphs)
            {
                morph_elements.push_back({
                    .position = pmx_vertex_morph.translation,
                    .vertex_index = static_cast<std::uint32_t>(pmx_vertex_morph.index),
                });
            }
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
            break;
        default:
            break;
        }

        mesh.geometry->add_morph_target(pmx_morph.name_jp, morph_elements);
    }
}

void mmd_loader::load_physics(world& world)
{
    std::vector<std::size_t> rigidbody_count(m_pmx.bones.size());
    std::vector<mat4f> rigidbody_transform(m_pmx.rigidbodies.size());

    for (std::size_t i = 0; i < m_pmx.rigidbodies.size(); ++i)
    {
        auto& pmx_rigidbody = m_pmx.rigidbodies[i];

        mat4f_simd transform = matrix::affine_transform(
            vector::set(1.0f, 1.0f, 1.0f, 0.0f),
            math::load(pmx_rigidbody.rotate),
            math::load(pmx_rigidbody.translate));

        math::store(transform, rigidbody_transform[i]);
        ++rigidbody_count[pmx_rigidbody.bone_index];
    }

    std::vector<entity> rigidbody_bones;
    rigidbody_bones.reserve(m_pmx.rigidbodies.size());
    for (std::size_t i = 0; i < m_pmx.rigidbodies.size(); ++i)
    {
        auto& pmx_rigidbody = m_pmx.rigidbodies[i];

        entity bone = m_bones[pmx_rigidbody.bone_index];
        if (rigidbody_count[pmx_rigidbody.bone_index] == 1)
        {
            world.add_component<rigidbody_component, collider_component>(bone);
        }
        else
        {
            // Workaround multiple rigid bodies are attached to a node.
            auto temp = world.create();
            world.add_component<
                transform_component,
                parent_component,
                rigidbody_component,
                collider_component,
                scene_component>(temp);

            auto& temp_parent = world.get_component<parent_component>(temp);
            temp_parent.parent = bone;
            m_bones.push_back(temp);

            bone = temp;
        }
        rigidbody_bones.push_back(bone);

        auto& bone_collider = world.get_component<collider_component>(bone);

        phy_collision_shape_desc shape = {};
        switch (pmx_rigidbody.shape)
        {
        case PMX_RIGIDBODY_SHAPE_TYPE_SPHERE:
            shape.type = PHY_COLLISION_SHAPE_TYPE_SPHERE;
            shape.sphere.radius = pmx_rigidbody.size[0];
            break;
        case PMX_RIGIDBODY_SHAPE_TYPE_BOX:
            shape.type = PHY_COLLISION_SHAPE_TYPE_BOX;
            shape.box.length = pmx_rigidbody.size[0] * 2.0f;
            shape.box.height = pmx_rigidbody.size[1] * 2.0f;
            shape.box.width = pmx_rigidbody.size[2] * 2.0f;
            break;
        case PMX_RIGIDBODY_SHAPE_TYPE_CAPSULE:
            shape.type = PHY_COLLISION_SHAPE_TYPE_CAPSULE;
            shape.capsule.radius = pmx_rigidbody.size[0];
            shape.capsule.height = pmx_rigidbody.size[1];
            break;
        default:
            break;
        };

        bone_collider.shapes.push_back({shape});

        auto& bone_rigidbody = world.get_component<rigidbody_component>(bone);
        bone_rigidbody.activation_state = PHY_ACTIVATION_STATE_DISABLE_DEACTIVATION;

        auto merge_reflector = [](const mat4f& rigidbody_world,
                                  const mat4f& transform_world) -> mat4f
        {
            mat4f result = rigidbody_world;
            result[3] = transform_world[3];
            return result;
        };

        switch (pmx_rigidbody.mode)
        {
        case PMX_RIGIDBODY_MODE_STATIC:
            bone_rigidbody.type = PHY_RIGIDBODY_TYPE_KINEMATIC;
            break;
        case PMX_RIGIDBODY_MODE_DYNAMIC:
            bone_rigidbody.type = PHY_RIGIDBODY_TYPE_DYNAMIC;
            break;
        case PMX_RIGIDBODY_MODE_MERGE:
            bone_rigidbody.type = PHY_RIGIDBODY_TYPE_DYNAMIC;
            bone_rigidbody.transform_reflector = merge_reflector;
            break;
        default:
            break;
        }

        bone_rigidbody.mass =
            pmx_rigidbody.mode == PMX_RIGIDBODY_MODE_STATIC ? 0.0f : pmx_rigidbody.mass;
        bone_rigidbody.linear_damping = pmx_rigidbody.linear_damping;
        bone_rigidbody.angular_damping = pmx_rigidbody.angular_damping;
        bone_rigidbody.restitution = pmx_rigidbody.repulsion;
        bone_rigidbody.friction = pmx_rigidbody.friction;
        bone_rigidbody.collision_group = static_cast<std::uint32_t>(1) << pmx_rigidbody.group;
        bone_rigidbody.collision_mask = pmx_rigidbody.collision_group;

        mat4f_simd bone_transform = math::load(m_bone_initial_transforms[pmx_rigidbody.bone_index]);
        mat4f_simd rigidbody_offset =
            matrix::mul(math::load(rigidbody_transform[i]), matrix::inverse(bone_transform));
        math::store(rigidbody_offset, bone_rigidbody.offset);
    }

    for (auto& pmx_joint : m_pmx.joints)
    {
        entity rigidbody_a = rigidbody_bones[pmx_joint.rigidbody_a_index];
        entity rigidbody_b = rigidbody_bones[pmx_joint.rigidbody_b_index];

        mat4f_simd joint_world = matrix::affine_transform(
            vector::set(1.0f, 1.0f, 1.0f, 0.0f),
            math::load(pmx_joint.rotate),
            math::load(pmx_joint.translate));

        mat4f_simd inverse_a = matrix::inverse_transform_without_scale(
            math::load(rigidbody_transform[pmx_joint.rigidbody_a_index]));
        mat4f_simd offset_a = matrix::mul(joint_world, inverse_a);

        vec4f_simd scale_a;
        vec4f_simd position_a;
        vec4f_simd rotation_a;
        matrix::decompose(offset_a, scale_a, rotation_a, position_a);

        mat4f_simd inverse_b = matrix::inverse_transform_without_scale(
            math::load(rigidbody_transform[pmx_joint.rigidbody_b_index]));
        mat4f_simd offset_b = matrix::mul(joint_world, inverse_b);

        vec4f_simd scale_b;
        vec4f_simd position_b;
        vec4f_simd rotation_b;
        matrix::decompose(offset_b, scale_b, rotation_b, position_b);

        joint joint = {
            .target = rigidbody_b,
            .min_linear = pmx_joint.translate_min,
            .max_linear = pmx_joint.translate_max,
            .min_angular = pmx_joint.rotate_min,
            .max_angular = pmx_joint.rotate_max,
        };

        math::store(position_a, joint.source_position);
        math::store(rotation_a, joint.source_rotation);
        math::store(position_b, joint.target_position);
        math::store(rotation_b, joint.target_rotation);

        for (std::size_t i = 0; i < 6; ++i)
        {
            joint.spring_enable[i] = pmx_joint.spring_translate_factor[i] != 0.0f;
            joint.stiffness[i] = pmx_joint.spring_translate_factor[i];
        }

        if (!world.has_component<joint_component>(rigidbody_a))
        {
            world.add_component<joint_component>(rigidbody_a);
        }
        world.get_component<joint_component>(rigidbody_a).joints.push_back(std::move(joint));
    }
}

void mmd_loader::load_animation(world& world)
{
    const auto& mmd_skeleton = world.get_component<const mmd_skeleton_component>(m_root);
    auto& mmd_animator = world.get_component<mmd_animator_component>(m_root);
    mmd_animator.motions.resize(mmd_skeleton.bones.size());

    std::map<std::string, std::size_t> name_to_index_map;
    for (std::size_t i = 0; i < mmd_skeleton.bones.size(); ++i)
    {
        name_to_index_map[mmd_skeleton.bones[i].name] = i;
    }

    auto get_bezier = [](const unsigned char* cp) -> bezier
    {
        int x0 = cp[0];
        int y0 = cp[4];
        int x1 = cp[8];
        int y1 = cp[12];

        return bezier{
            {static_cast<float>(x0) / 127.0f, static_cast<float>(y0) / 127.0f},
            {static_cast<float>(x1) / 127.0f, static_cast<float>(y1) / 127.0f},
        };
    };

    for (const auto& vmd_motion : m_vmd.motions)
    {
        auto iter = name_to_index_map.find(vmd_motion.bone_name);
        if (iter != name_to_index_map.end())
        {
            mmd_animation_key key = {
                .frame = static_cast<std::int32_t>(vmd_motion.frame_index),
                .translate = vmd_motion.translate,
                .rotate = vmd_motion.rotate,
                .tx_bezier = get_bezier(vmd_motion.interpolation.data() + 0),
                .ty_bezier = get_bezier(vmd_motion.interpolation.data() + 1),
                .tz_bezier = get_bezier(vmd_motion.interpolation.data() + 2),
                .r_bezier = get_bezier(vmd_motion.interpolation.data() + 3),
            };

            mmd_animator.motions[iter->second].animation_keys.push_back(key);
        }
    }

    for (auto& motion : mmd_animator.motions)
    {
        std::sort(
            motion.animation_keys.begin(),
            motion.animation_keys.end(),
            [](const mmd_animation_key& a, const mmd_animation_key& b)
            {
                return a.frame < b.frame;
            });
    }

    for (const auto& vmd_ik : m_vmd.iks)
    {
        for (const auto& info : vmd_ik.infos)
        {
            auto iter = name_to_index_map.find(info.name);
            if (iter != name_to_index_map.end())
            {
                mmd_ik_key key = {
                    .frame = static_cast<std::int32_t>(vmd_ik.frame),
                    .enable = info.enable != 0,
                };
                mmd_animator.motions[iter->second].ik_keys.push_back(key);
            }
        }
    }

    for (auto& motion : mmd_animator.motions)
    {
        std::sort(
            motion.ik_keys.begin(),
            motion.ik_keys.end(),
            [](const auto& a, const auto& b)
            {
                return a.frame < b.frame;
            });
    }

    const auto& mesh = world.get_component<const mesh_component>(m_root);
    mmd_animator.morphs.resize(mesh.geometry->get_morph_target_count());

    for (auto& morph : m_vmd.morphs)
    {
        auto iter = std::find_if(
            m_pmx.morphs.begin(),
            m_pmx.morphs.end(),
            [&](const auto& m)
            {
                return m.name_jp == morph.morph_name;
            });

        if (iter != m_pmx.morphs.end())
        {
            std::size_t index = std::distance(m_pmx.morphs.begin(), iter);
            mmd_animator.morphs[index].morph_keys.push_back({
                .frame = static_cast<std::int32_t>(morph.frame),
                .weight = morph.weight,
            });
        }
    }

    for (auto& morph : mmd_animator.morphs)
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
} // namespace violet

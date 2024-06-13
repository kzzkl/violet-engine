#include "mmd_animation.hpp"
#include "common/log.hpp"
#include "components/mesh.hpp"
#include "graphics/graphics_module.hpp"

namespace violet::sample
{
class mmd_skeleton_info : public component_info_default<mmd_skeleton>
{
public:
    mmd_skeleton_info(render_device* device) : m_device(device) {}

    virtual void construct(actor* owner, void* target) override
    {
        new (target) mmd_skeleton(m_device);
    }

private:
    render_device* m_device;
};

class skin_pass : public rdg_compute_pass
{
public:
    skin_pass()
    {
        set_shader("mmd-viewer/shaders/skinning.comp.spv");
        set_parameter_layout({
            {mmd_parameter_layout::skeleton, RDG_PASS_PARAMETER_FLAG_NONE}
        });
    }

    void execute(rhi_command* command, rdg_context* context) override
    {
        command->set_compute_pipeline(get_pipeline());
        for (auto& dispatch : context->get_dispatches(this))
        {
            command->set_compute_parameter(0, dispatch.parameter);
            command->dispatch(dispatch.x, dispatch.y, dispatch.z);
        }
    }
};

mmd_animation::mmd_animation() : engine_module("mmd animation"), m_skin_pass(nullptr)
{
}

bool mmd_animation::initialize(const dictionary& config)
{
    render_device* device = get_module<graphics_module>().get_device();

    get_world().register_component<mmd_animator>();
    get_world().register_component<mmd_morph>();
    get_world().register_component<mmd_skeleton, mmd_skeleton_info>(device);

    m_skin_graph = std::make_unique<render_graph>();
    m_skin_pass = m_skin_graph->add_pass<skin_pass>("skin pass");
    m_skin_graph->compile(device);

    m_skin_context = m_skin_graph->create_context();

    return true;
}

void mmd_animation::evaluate(float t, float weight)
{
    view<mmd_skeleton, mmd_animator, mmd_morph> view(get_world());
    view.each(
        [=, this](mmd_skeleton& skeleton, mmd_animator& animator, mmd_morph& morph)
        {
            evaluate_motion(skeleton, animator, t, weight);
        });
    view.each(
        [=, this](mmd_skeleton& skeleton, mmd_animator& animator, mmd_morph& morph)
        {
            evaluate_ik(skeleton, animator, t, weight);
        });
    view.each(
        [=, this](mmd_skeleton& skeleton, mmd_animator& animator, mmd_morph& morph)
        {
            evaluate_morph(morph, animator, t);
        });
}

void mmd_animation::update(bool after_physics)
{
    view<mmd_skeleton, mmd_animator> view(get_world());
    view.each(
        [=, this](mmd_skeleton& skeleton, mmd_animator& animator)
        {
            for (std::size_t i = 0; i < skeleton.sorted_bones.size(); ++i)
            {
                auto& bone = skeleton.bones[skeleton.sorted_bones[i]];
                auto& motion = animator.motions[skeleton.sorted_bones[i]];
                if (bone.deform_after_physics != after_physics)
                    continue;

                update_local(bone, motion);
            }

            for (std::size_t i = 0; i < skeleton.sorted_bones.size(); ++i)
            {
                auto& bone = skeleton.bones[skeleton.sorted_bones[i]];
                auto& motion = animator.motions[skeleton.sorted_bones[i]];
                if (bone.deform_after_physics != after_physics)
                    continue;

                if (bone.inherit_index != -1)
                {
                    update_inherit(
                        bone,
                        motion,
                        skeleton.bones[bone.inherit_index],
                        animator.motions[bone.inherit_index]);
                }

                if (bone.ik_solver)
                    update_ik(skeleton, animator, bone, motion);
            }
        });
}

void mmd_animation::skinning()
{
    view<mmd_skeleton, mesh, transform> view(get_world());
    view.each(
        [&](mmd_skeleton& model_skeleton, mesh& model_mesh, transform& model_transform)
        {
            matrix4 world_to_local =
                matrix::inverse_transform(matrix::load(model_transform.get_world_matrix()));

            for (std::size_t i = 0; i < model_skeleton.bones.size(); ++i)
            {
                auto bone_transform = model_skeleton.bones[i].transform;

                matrix4 final_transform = matrix::load(bone_transform->get_world_matrix());
                final_transform = matrix::mul(final_transform, world_to_local);

                matrix4 initial_inverse = matrix::load(model_skeleton.bones[i].initial_inverse);
                final_transform = matrix::mul(initial_inverse, final_transform);

                mmd_skinning_bone data;
                matrix::store(matrix::transpose(final_transform), data.offset);
                vector::store(quaternion::from_matrix(final_transform), data.quaternion);

                model_skeleton.set_bone(i, data);
            }

            rdg_dispatch dispatch = {};
            dispatch.parameter = model_skeleton.get_parameter();
            dispatch.x = (model_mesh.get_geometry()->get_vertex_count() + 255) / 256;
            dispatch.y = 1;
            dispatch.z = 1;
            m_skin_context->add_dispatch(m_skin_pass, dispatch);
        });

    render_device* device = get_module<graphics_module>().get_device();
    rhi_command* command = device->allocate_command();
    m_skin_graph->execute(command, m_skin_context.get());
    device->execute({command}, {}, {}, nullptr);

    m_skin_context->reset();
}

void mmd_animation::evaluate_motion(
    mmd_skeleton& skeleton,
    mmd_animator& animator,
    float t,
    float weight)
{
    for (auto& bone : skeleton.bones)
    {
        bone.position = bone.initial_position;
        bone.rotation = bone.initial_rotation;
        bone.scale = bone.initial_scale;
        bone.inherit_translation = {0.0f, 0.0f, 0.0f};
        bone.inherit_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        if (bone.ik_link)
            bone.ik_link->rotate = {0.0f, 0.0f, 0.0f, 1.0f};
    }

    for (std::size_t i = 0; i < skeleton.bones.size(); ++i)
    {
        auto& motion = animator.motions[i];

        if (motion.animation_keys.empty())
        {
            motion.translation = {0.0f, 0.0f, 0.0f};
            motion.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
            continue;
        }

        auto bound = bound_key(motion.animation_keys, static_cast<std::int32_t>(t), motion.offset);

        vector4 translate;
        vector4 rotate;

        if (bound == motion.animation_keys.end())
        {
            translate = vector::load(motion.animation_keys.back().translate);
            rotate = vector::load(motion.animation_keys.back().rotate);
        }
        else if (bound == motion.animation_keys.begin())
        {
            translate = vector::load(bound->translate);
            rotate = vector::load(bound->rotate);
        }
        else
        {
            const auto& key0 = *(bound - 1);
            const auto& key1 = *bound;

            float time = (t - key0.frame) / static_cast<float>(key1.frame - key0.frame);

            translate = vector::lerp(
                vector::load(key0.translate),
                vector::load(key1.translate),
                vector::set(
                    key1.tx_bezier.evaluate(time),
                    key1.ty_bezier.evaluate(time),
                    key1.tz_bezier.evaluate(time),
                    0.0f));

            rotate = quaternion::slerp(
                vector::load(key0.rotate),
                vector::load(key1.rotate),
                key1.r_bezier.evaluate(time));

            motion.offset = std::distance(motion.animation_keys.cbegin(), bound);
        }

        if (weight != 1.0f)
        {
            rotate = quaternion::slerp(vector::load(motion.base_rotation), rotate, weight);
            translate = vector::lerp(vector::load(motion.base_translation), translate, weight);
        }

        vector::store(translate, motion.translation);
        vector::store(rotate, motion.rotation);
    }
}

void mmd_animation::evaluate_ik(
    mmd_skeleton& skeleton,
    mmd_animator& animator,
    float t,
    float weight)
{
    for (std::size_t i = 0; i < skeleton.bones.size(); ++i)
    {
        if (skeleton.bones[i].ik_solver == nullptr)
            continue;

        auto& bone = skeleton.bones[i];
        auto& motion = animator.motions[i];

        if (motion.ik_keys.empty())
        {
            bone.ik_solver->enable = true;
            continue;
        }

        auto bound =
            bound_key(motion.ik_keys, static_cast<std::int32_t>(t), bone.ik_solver->offset);
        bool enable = true;
        if (bound == motion.ik_keys.end())
        {
            enable = motion.ik_keys.back().enable;
        }
        else
        {
            enable = motion.ik_keys.front().enable;
            if (bound != motion.ik_keys.begin())
            {
                const auto& key = *(bound - 1);
                enable = key.enable;

                bone.ik_solver->offset = std::distance(motion.ik_keys.cbegin(), bound);
            }
        }

        if (weight == 1.0f)
        {
            bone.ik_solver->enable = enable;
        }
        else
        {
            if (weight < 1.0f)
                bone.ik_solver->enable = bone.ik_solver->base_animation;
            else
                bone.ik_solver->enable = enable;
        }
    }
}

void mmd_animation::evaluate_morph(mmd_morph& morph, mmd_animator& animator, float t)
{
    std::memset(
        morph.vertex_morph_result->get_buffer(),
        0,
        morph.vertex_morph_result->get_buffer_size());

    for (std::size_t i = 0; i < morph.morphs.size(); ++i)
    {
        auto& animator_morph = animator.morphs[i];

        if (animator_morph.morph_keys.empty())
            continue;

        auto bound = bound_key(
            animator_morph.morph_keys,
            static_cast<std::int32_t>(t),
            animator_morph.offset);

        float weight = 0.0f;
        if (bound == animator_morph.morph_keys.end())
        {
            weight = animator_morph.morph_keys.back().weight;
        }
        else if (bound == animator_morph.morph_keys.begin())
        {
            weight = bound->weight;
        }
        else
        {
            const auto& key0 = *(bound - 1);
            const auto& key1 = *bound;

            float offset = (t - key0.frame) / (key1.frame - key0.frame);
            weight = key0.weight + (key1.weight - key0.weight) * offset;

            animator_morph.offset = std::distance(animator_morph.morph_keys.cbegin(), bound);
        }

        if (weight != 0.0f)
            morph.evaluate(i, weight);
    }
}

void mmd_animation::update_inherit(
    mmd_skeleton::bone& bone,
    mmd_animator::motion& motion,
    mmd_skeleton::bone& inherit_bone,
    mmd_animator::motion& inherit_motion)
{
    if (bone.is_inherit_rotation)
    {
        vector4 rotate;
        if (!bone.inherit_local_flag && inherit_bone.inherit_index != -1)
            rotate = vector::load(inherit_bone.inherit_rotation);
        else
            rotate = vector::load(inherit_motion.rotation);

        if (inherit_bone.ik_link)
            rotate = quaternion::mul(vector::load(inherit_bone.ik_link->rotate), rotate);

        rotate = quaternion::slerp(quaternion::identity<vector4>(), rotate, bone.inherit_weight);
        vector::store(rotate, bone.inherit_rotation);
    }

    if (bone.is_inherit_translation)
    {
        vector4 translate;
        if (!bone.inherit_local_flag && inherit_bone.inherit_index != -1)
        {
            translate = vector::load(inherit_bone.inherit_translation);
        }
        else
        {
            translate = vector::sub(
                vector::load(inherit_bone.position),
                vector::load(inherit_bone.initial_position));
        }
        translate = vector::mul(translate, bone.inherit_weight);
        vector::store(translate, bone.inherit_translation);
    }

    update_local(bone, motion);
}

void mmd_animation::update_ik(
    mmd_skeleton& skeleton,
    mmd_animator& animator,
    mmd_skeleton::bone& bone,
    mmd_animator::motion& motion)
{
    if (bone.ik_solver->target_index == -1 || !bone.ik_solver->enable)
        return;

    for (std::size_t link : bone.ik_solver->links)
    {
        skeleton.bones[link].ik_link->prev_angle = {0.0f, 0.0f, 0.0f};
        skeleton.bones[link].ik_link->rotate = {0.0f, 0.0f, 0.0f, 1.0f};
        skeleton.bones[link].ik_link->plane_mode_angle = 0.0f;
        update_local(skeleton.bones[link], animator.motions[link]);
    }

    float max_dist = std::numeric_limits<float>::max();
    for (std::uint32_t i = 0; i < bone.ik_solver->iteration_count; i++)
    {
        ik_solve_core(skeleton, animator, bone, i);

        vector4 target_position = vector::load(
            skeleton.bones[bone.ik_solver->target_index].transform->get_world_matrix()[3]);
        vector4 ik_position = vector::load(bone.transform->get_world_matrix()[3]);
        float dist = vector::length(vector::sub(target_position, ik_position));
        if (dist < max_dist)
        {
            max_dist = dist;
            for (std::size_t link : bone.ik_solver->links)
                skeleton.bones[link].ik_link->save_rotate = skeleton.bones[link].ik_link->rotate;
        }
        else
        {
            for (std::size_t link : bone.ik_solver->links)
            {
                skeleton.bones[link].ik_link->rotate = skeleton.bones[link].ik_link->save_rotate;
                update_local(skeleton.bones[link], animator.motions[link]);
            }
            break;
        }
    }
}

void mmd_animation::ik_solve_core(
    mmd_skeleton& skeleton,
    mmd_animator& animator,
    mmd_skeleton::bone& bone,
    std::size_t iteration)
{
    vector4 ik_position = vector::load(bone.transform->get_world_matrix()[3]);
    for (std::size_t i = 0; i < bone.ik_solver->links.size(); ++i)
    {
        if (bone.ik_solver->links[i] == bone.ik_solver->target_index)
            continue;

        auto& link_bone = skeleton.bones[bone.ik_solver->links[i]];
        if (link_bone.ik_link->enable_limit)
        {
            if ((link_bone.ik_link->limit_min[0] != 0.0f ||
                 link_bone.ik_link->limit_max[0] != 0.0f) &&
                (link_bone.ik_link->limit_min[1] == 0.0f ||
                 link_bone.ik_link->limit_max[1] == 0.0f) &&
                (link_bone.ik_link->limit_min[2] == 0.0f ||
                 link_bone.ik_link->limit_max[2] == 0.0f))
            {
                ik_solve_plane(skeleton, animator, bone, link_bone, 0, iteration);
                continue;
            }
            else if (
                (link_bone.ik_link->limit_min[0] == 0.0f ||
                 link_bone.ik_link->limit_max[0] == 0.0f) &&
                (link_bone.ik_link->limit_min[1] != 0.0f ||
                 link_bone.ik_link->limit_max[1] != 0.0f) &&
                (link_bone.ik_link->limit_min[2] == 0.0f ||
                 link_bone.ik_link->limit_max[2] == 0.0f))
            {
                ik_solve_plane(skeleton, animator, bone, link_bone, 1, iteration);
                continue;
            }
            else if (
                (link_bone.ik_link->limit_min[0] == 0.0f ||
                 link_bone.ik_link->limit_max[0] == 0.0f) &&
                (link_bone.ik_link->limit_min[1] == 0.0f ||
                 link_bone.ik_link->limit_max[1] == 0.0f) &&
                (link_bone.ik_link->limit_min[2] != 0.0f ||
                 link_bone.ik_link->limit_max[2] != 0.0f))
            {
                ik_solve_plane(skeleton, animator, bone, link_bone, 2, iteration);
                continue;
            }
        }
        vector4 target_position = vector::load(
            skeleton.bones[bone.ik_solver->target_index].transform->get_world_matrix()[3]);

        matrix4 link_inverse = matrix::inverse_transform_no_scale(
            matrix::load(link_bone.transform->get_world_matrix()));

        vector4 link_ik_position = matrix::mul(ik_position, link_inverse);
        vector4 link_target_position = matrix::mul(target_position, link_inverse);

        vector4 link_ik_vec = vector::normalize(link_ik_position);
        vector4 link_target_vec = vector::normalize(link_target_position);

        float dot = vector::dot(link_ik_vec, link_target_vec);
        dot = clamp(dot, -1.0f, 1.0f);

        float angle = std::acos(dot);
        float angle_deg = to_degrees(angle);
        if (angle_deg < 1.0e-3f)
            continue;

        angle = clamp(angle, -bone.ik_solver->limit, bone.ik_solver->limit);
        vector4 cross = vector::normalize(vector::cross(link_target_vec, link_ik_vec));
        vector4 rotate = quaternion::from_axis_angle(cross, angle);

        if (link_bone.ik_link->enable_limit)
        {
        }

        vector4 rotation = quaternion::mul(
            vector::load(animator.motions[bone.ik_solver->links[i]].rotation),
            vector::load(link_bone.rotation));

        vector4 link_rotate = vector::load(link_bone.ik_link->rotate);
        link_rotate = quaternion::mul(link_rotate, rotation);
        link_rotate = quaternion::mul(link_rotate, rotate);
        link_rotate = quaternion::mul(link_rotate, quaternion::inverse(rotation));
        vector::store(link_rotate, link_bone.ik_link->rotate);

        update_local(link_bone, animator.motions[bone.ik_solver->links[i]]);
    }
}

void mmd_animation::ik_solve_plane(
    mmd_skeleton& skeleton,
    mmd_animator& animator,
    mmd_skeleton::bone& bone,
    mmd_skeleton::bone& ik_link,
    std::uint8_t axis,
    std::size_t iteration)
{
    vector4 rotate_axis;
    switch (axis)
    {
    case 0: // x axis
        rotate_axis = math::identity_row_0;
        break;
    case 1: // y axis
        rotate_axis = math::identity_row_1;
        break;
    case 2: // z axis
        rotate_axis = math::identity_row_2;
        break;
    default:
        return;
    }

    vector4 ik_position = vector::load(bone.transform->get_world_matrix()[3]);
    vector4 target_position =
        vector::load(skeleton.bones[bone.ik_solver->target_index].transform->get_world_matrix()[3]);
    matrix4 link_inverse =
        matrix::inverse_transform_no_scale(matrix::load(ik_link.transform->get_world_matrix()));

    vector4 link_ik_vec = matrix::mul(ik_position, link_inverse);
    link_ik_vec = vector::normalize(link_ik_vec);

    vector4 link_target_vec = matrix::mul(target_position, link_inverse);
    link_target_vec = vector::normalize(link_target_vec);

    float dot = vector::dot(link_ik_vec, link_target_vec);
    dot = clamp(dot, -1.0f, 1.0f);

    float angle = std::acos(dot);
    angle = clamp(angle, -bone.ik_solver->limit, bone.ik_solver->limit);

    vector4 rotate1 = quaternion::from_axis_angle(rotate_axis, angle);
    vector4 target_vec1 = quaternion::mul_vec(rotate1, link_target_vec);
    float dot1 = vector::dot(target_vec1, link_ik_vec);

    vector4 rotate2 = quaternion::from_axis_angle(rotate_axis, -angle);
    vector4 target_vec2 = quaternion::mul_vec(rotate2, link_target_vec);
    float dot2 = vector::dot(target_vec2, link_ik_vec);

    float new_angle = ik_link.ik_link->plane_mode_angle;
    if (dot1 > dot2)
        new_angle += angle;
    else
        new_angle -= angle;

    if (iteration == 0)
    {
        if (new_angle < ik_link.ik_link->limit_min[axis] ||
            new_angle > ik_link.ik_link->limit_max[axis])
        {
            if (-new_angle > ik_link.ik_link->limit_min[axis] &&
                -new_angle < ik_link.ik_link->limit_max[axis])
            {
                new_angle *= -1.0f;
            }
            else
            {
                auto halfRad =
                    (ik_link.ik_link->limit_min[axis] + ik_link.ik_link->limit_max[axis]) * 0.5f;
                if (std::abs(halfRad - new_angle) > std::abs(halfRad + new_angle))
                    new_angle *= -1.0f;
            }
        }
    }

    new_angle =
        clamp(new_angle, ik_link.ik_link->limit_min[axis], ik_link.ik_link->limit_max[axis]);
    ik_link.ik_link->plane_mode_angle = new_angle;

    vector4 rotation = quaternion::mul(
        vector::load(animator.motions[ik_link.index].rotation),
        vector::load(skeleton.bones[ik_link.index].rotation));
    vector4 link_rotate = quaternion::from_axis_angle(rotate_axis, new_angle);
    link_rotate = quaternion::mul(link_rotate, quaternion::inverse(rotation));

    vector::store(link_rotate, ik_link.ik_link->rotate);

    update_local(skeleton.bones[ik_link.index], animator.motions[ik_link.index]);
}

void mmd_animation::update_local(mmd_skeleton::bone& bone, mmd_animator::motion& motion)
{
    vector4 translate = vector::add(vector::load(motion.translation), vector::load(bone.position));
    if (bone.is_inherit_translation)
        translate = vector::add(translate, vector::load(bone.inherit_translation));

    vector4 rotation = quaternion::mul(vector::load(motion.rotation), vector::load(bone.rotation));
    if (bone.ik_link)
        rotation = quaternion::mul(vector::load(bone.ik_link->rotate), rotation);
    if (bone.is_inherit_rotation)
        rotation = quaternion::mul(rotation, vector::load(bone.inherit_rotation));

    float3 t;
    vector::store(translate, t);
    bone.transform->set_position(t);

    float4 r;
    vector::store(rotation, r);
    bone.transform->set_rotation(r);

    bone.transform->set_scale(bone.scale);
}
} // namespace violet::sample
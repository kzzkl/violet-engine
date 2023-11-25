#include "mmd_animation.hpp"
#include "common/log.hpp"

namespace violet::sample
{
mmd_animation::mmd_animation() : engine_system("mmd animation")
{
}

bool mmd_animation::initialize(const dictionary& config)
{
    get_world().register_component<mmd_animator>();

    return true;
}

void mmd_animation::evaluate(float t, float weight)
{
    view<mmd_skeleton, mmd_animator> view(get_world());
    view.each(
        [=, this](mmd_skeleton& skeleton, mmd_animator& animator)
        {
            evaluate_motion(skeleton, animator, t, weight);
        });
    view.each(
        [=, this](mmd_skeleton& skeleton, mmd_animator& animator)
        {
            evaluate_ik(skeleton, animator, t, weight);
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

        float4_simd translate;
        float4_simd rotate;

        if (bound == motion.animation_keys.end())
        {
            translate = simd::load(motion.animation_keys.back().translate);
            rotate = simd::load(motion.animation_keys.back().rotate);
        }
        else if (bound == motion.animation_keys.begin())
        {
            translate = simd::load(bound->translate);
            rotate = simd::load(bound->rotate);
        }
        else
        {
            const auto& key0 = *(bound - 1);
            const auto& key1 = *bound;

            float time = (t - key0.frame) / static_cast<float>(key1.frame - key0.frame);

            translate = vector_simd::lerp(
                simd::load(key0.translate),
                simd::load(key1.translate),
                simd::set(
                    key1.tx_bezier.evaluate(time),
                    key1.ty_bezier.evaluate(time),
                    key1.tz_bezier.evaluate(time),
                    0.0f));

            rotate = quaternion_simd::slerp(
                simd::load(key0.rotate),
                simd::load(key1.rotate),
                key1.r_bezier.evaluate(time));

            motion.offset = std::distance(motion.animation_keys.cbegin(), bound);
        }

        if (weight != 1.0f)
        {
            rotate = quaternion_simd::slerp(simd::load(motion.base_rotation), rotate, weight);
            translate = vector_simd::lerp(simd::load(motion.base_translation), translate, weight);
        }

        simd::store(translate, motion.translation);
        simd::store(rotate, motion.rotation);
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

void mmd_animation::update_inherit(
    mmd_skeleton::bone& bone,
    mmd_animator::motion& motion,
    mmd_skeleton::bone& inherit_bone,
    mmd_animator::motion& inherit_motion)
{
    if (bone.is_inherit_rotation)
    {
        float4_simd rotate;
        if (!bone.inherit_local_flag && inherit_bone.inherit_index != -1)
            rotate = simd::load(inherit_bone.inherit_rotation);
        else
            rotate = simd::load(inherit_motion.rotation);

        if (inherit_bone.ik_link)
            rotate = quaternion_simd::mul(simd::load(inherit_bone.ik_link->rotate), rotate);

        rotate = quaternion_simd::slerp(quaternion_simd::identity(), rotate, bone.inherit_weight);
        simd::store(rotate, bone.inherit_rotation);
    }

    if (bone.is_inherit_translation)
    {
        float4_simd translate;
        if (!bone.inherit_local_flag && inherit_bone.inherit_index != -1)
        {
            translate = simd::load(inherit_bone.inherit_translation);
        }
        else
        {
            translate = vector_simd::sub(
                simd::load(inherit_bone.position),
                simd::load(inherit_bone.initial_position));
        }
        translate = vector_simd::mul(translate, bone.inherit_weight);
        simd::store(translate, bone.inherit_translation);
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

        float4_simd target_position = simd::load(
            skeleton.bones[bone.ik_solver->target_index].transform->get_world_matrix()[3]);
        float4_simd ik_position = simd::load(bone.transform->get_world_matrix()[3]);
        float dist = vector_simd::length(vector_simd::sub(target_position, ik_position));
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
    float4_simd ik_position = simd::load(bone.transform->get_world_matrix()[3]);
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
        float4_simd target_position = simd::load(
            skeleton.bones[bone.ik_solver->target_index].transform->get_world_matrix()[3]);

        float4x4_simd link_inverse = matrix_simd::inverse_transform_no_scale(
            simd::load(link_bone.transform->get_world_matrix()));

        float4_simd link_ik_position = matrix_simd::mul(ik_position, link_inverse);
        float4_simd link_target_position = matrix_simd::mul(target_position, link_inverse);

        float4_simd link_ik_vec = vector_simd::normalize_vec3(link_ik_position);
        float4_simd link_target_vec = vector_simd::normalize_vec3(link_target_position);

        float dot = vector_simd::dot(link_ik_vec, link_target_vec);
        dot = clamp(dot, -1.0f, 1.0f);

        float angle = std::acos(dot);
        float angle_deg = to_degrees(angle);
        if (angle_deg < 1.0e-3f)
            continue;

        angle = clamp(angle, -bone.ik_solver->limit, bone.ik_solver->limit);
        float4_simd cross =
            vector_simd::normalize_vec3(vector_simd::cross(link_target_vec, link_ik_vec));
        float4_simd rotate = quaternion_simd::rotation_axis(cross, angle);

        if (link_bone.ik_link->enable_limit)
        {
        }

        float4_simd rotation = quaternion_simd::mul(
            simd::load(animator.motions[bone.ik_solver->links[i]].rotation),
            simd::load(link_bone.rotation));

        float4_simd link_rotate = simd::load(link_bone.ik_link->rotate);
        link_rotate = quaternion_simd::mul(link_rotate, rotation);
        link_rotate = quaternion_simd::mul(link_rotate, rotate);
        link_rotate = quaternion_simd::mul(link_rotate, quaternion_simd::inverse(rotation));
        simd::store(link_rotate, link_bone.ik_link->rotate);

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
    float4_simd rotate_axis;
    switch (axis)
    {
    case 0: // x axis
        rotate_axis = simd::identity_row_v<0>;
        break;
    case 1: // y axis
        rotate_axis = simd::identity_row_v<1>;
        break;
    case 2: // z axis
        rotate_axis = simd::identity_row_v<2>;
        break;
    default:
        return;
    }

    float4_simd ik_position = simd::load(bone.transform->get_world_matrix()[3]);
    float4_simd target_position =
        simd::load(skeleton.bones[bone.ik_solver->target_index].transform->get_world_matrix()[3]);
    float4x4_simd link_inverse =
        matrix_simd::inverse_transform_no_scale(simd::load(ik_link.transform->get_world_matrix()));

    float4_simd link_ik_vec = matrix_simd::mul(ik_position, link_inverse);
    link_ik_vec = vector_simd::normalize_vec3(link_ik_vec);

    float4_simd link_target_vec = matrix_simd::mul(target_position, link_inverse);
    link_target_vec = vector_simd::normalize_vec3(link_target_vec);

    float dot = vector_simd::dot(link_ik_vec, link_target_vec);
    dot = clamp(dot, -1.0f, 1.0f);

    float angle = std::acos(dot);
    angle = clamp(angle, -bone.ik_solver->limit, bone.ik_solver->limit);

    float4_simd rotate1 = quaternion_simd::rotation_axis(rotate_axis, angle);
    float4_simd target_vec1 = quaternion_simd::mul_vec(rotate1, link_target_vec);
    float dot1 = vector_simd::dot(target_vec1, link_ik_vec);

    float4_simd rotate2 = quaternion_simd::rotation_axis(rotate_axis, -angle);
    float4_simd target_vec2 = quaternion_simd::mul_vec(rotate2, link_target_vec);
    float dot2 = vector_simd::dot(target_vec2, link_ik_vec);

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

    float4_simd rotation = quaternion_simd::mul(
        simd::load(animator.motions[ik_link.index].rotation),
        simd::load(skeleton.bones[ik_link.index].rotation));
    float4_simd link_rotate = quaternion_simd::rotation_axis(rotate_axis, new_angle);
    link_rotate = quaternion_simd::mul(link_rotate, quaternion_simd::inverse(rotation));

    simd::store(link_rotate, ik_link.ik_link->rotate);

    update_local(skeleton.bones[ik_link.index], animator.motions[ik_link.index]);
}

void mmd_animation::update_local(mmd_skeleton::bone& bone, mmd_animator::motion& motion)
{
    float4_simd translate =
        vector_simd::add(simd::load(motion.translation), simd::load(bone.position));
    if (bone.is_inherit_translation)
        translate = vector_simd::add(translate, simd::load(bone.inherit_translation));

    float4_simd rotation =
        quaternion_simd::mul(simd::load(motion.rotation), simd::load(bone.rotation));
    if (bone.ik_link)
        rotation = quaternion_simd::mul(simd::load(bone.ik_link->rotate), rotation);
    if (bone.is_inherit_rotation)
        rotation = quaternion_simd::mul(rotation, simd::load(bone.inherit_rotation));

    bone.transform->set_position(translate);
    bone.transform->set_rotation(rotation);
    bone.transform->set_scale(bone.scale);
}
} // namespace violet::sample
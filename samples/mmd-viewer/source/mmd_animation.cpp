#include "mmd_animation.hpp"
#include "components/mesh_component.hpp"
#include "components/transform_component.hpp"
#include "math/matrix.hpp"
#include "math/quaternion.hpp"
#include "math/vector.hpp"
#include "scene/transform_system.hpp"

namespace violet
{
namespace
{
template <typename Key>
auto bound_key(const std::vector<Key>& keys, std::int32_t t, std::size_t start)
{
    if (keys.empty() || keys.size() < start)
    {
        return keys.end();
    }

    return std::upper_bound(
        keys.begin(),
        keys.end(),
        t,
        [](std::int32_t lhs, const Key& rhs)
        {
            return lhs < rhs.frame;
        });
}

float normalize_angle(float angle)
{
    while (angle >= math::TWO_PI)
    {
        angle -= math::TWO_PI;
    }

    while (angle < 0)
    {
        angle += math::TWO_PI;
    }

    return angle;
}

float diff_angle(float a, float b)
{
    float diff = normalize_angle(a) - normalize_angle(b);
    if (diff > math::PI)
    {
        return diff - math::TWO_PI;
    }

    if (diff < -math::PI)
    {
        return diff + math::TWO_PI;
    }

    return diff;
}

float clamp_angle(float angle, float min_angle, float max_angle)
{
    if (min_angle == max_angle)
    {
        return min_angle;
    }

    float ret = angle;
    while (ret < min_angle)
    {
        ret += math::TWO_PI;
    }
    if (ret < max_angle)
    {
        return ret;
    }

    while (ret > max_angle)
    {
        ret -= math::TWO_PI;
    }
    if (ret > min_angle)
    {
        return ret;
    }

    float min_diff = std::abs(diff_angle(min_angle, ret));
    float max_diff = std::abs(diff_angle(max_angle, ret));
    if (min_diff < max_diff)
    {
        return min_angle;
    }

    return max_angle;
}

vec3f decompose(const mat4f& m, const vec3f& before)
{
    vec3f r;
    float sy = -m[0][2];
    const float e = 1.0e-6f;
    if ((1.0f - std::abs(sy)) < e)
    {
        r.y = std::asin(sy);

        float sx = std::sin(before.x);
        float sz = std::sin(before.z);
        if (std::abs(sx) < std::abs(sz))
        {
            float cx = std::cos(before.x);
            if (cx > 0)
            {
                r.x = 0;
                r.z = std::asin(-m[1][0]);
            }
            else
            {
                r.x = math::PI;
                r.z = std::asin(m[1][0]);
            }
        }
        else
        {
            float cz = std::cos(before.z);
            if (cz > 0)
            {
                r.z = 0;
                r.x = std::asin(-m[2][1]);
            }
            else
            {
                r.z = math::PI;
                r.x = std::asin(m[2][1]);
            }
        }
    }
    else
    {
        r.x = std::atan2(m[1][2], m[2][2]);
        r.y = std::asin(-m[0][2]);
        r.z = std::atan2(m[0][1], m[0][0]);
    }

    const float pi = math::PI;
    vec3f tests[] = {
        {r.x + pi, pi - r.y, r.z + pi},
        {r.x + pi, pi - r.y, r.z - pi},
        {r.x + pi, -pi - r.y, r.z + pi},
        {r.x + pi, -pi - r.y, r.z - pi},
        {r.x - pi, pi - r.y, r.z + pi},
        {r.x - pi, pi - r.y, r.z - pi},
        {r.x - pi, -pi - r.y, r.z + pi},
        {r.x - pi, -pi - r.y, r.z - pi},
    };

    float err_x = std::abs(diff_angle(r.x, before.x));
    float err_y = std::abs(diff_angle(r.y, before.y));
    float err_z = std::abs(diff_angle(r.z, before.z));
    float min_err = err_x + err_y + err_z;
    for (const auto test : tests)
    {
        float err = std::abs(diff_angle(test.x, before.x)) +
                    std::abs(diff_angle(test.y, before.y)) + std::abs(diff_angle(test.z, before.z));
        if (err < min_err)
        {
            min_err = err;
            r = test;
        }
    }
    return r;
}
} // namespace

mmd_animation::mmd_animation()
    : system("mmd animation")
{
}

bool mmd_animation::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<mmd_animator_component>();

    auto& task_graph = get_task_graph();

    auto& transform_group = task_graph.get_group("Transform");
    auto& animation_phase_1 = task_graph.add_task();
    animation_phase_1.set_name("MMD Animation Phase 1")
        .add_dependency(transform_group)
        .set_execute(
            [this]()
            {
                static float total_time = 0.0f;
                total_time += get_timer().get_frame_delta();
                evaluate(total_time * 30.0f);
                update(false);

                get_system<transform_system>().update_transform();
            });

    auto& physics_group = task_graph.get_group("Physics");
    physics_group.add_dependency(animation_phase_1);

    auto& animation_phase_2 = task_graph.add_task();
    animation_phase_2.set_name("MMD Animation Phase 2")
        .add_dependency(physics_group)
        .set_execute(
            [this]()
            {
                update(true);

                get_system<transform_system>().update_transform();
            });

    auto& rendering_group = task_graph.get_group("Rendering");
    rendering_group.add_dependency(animation_phase_2);

    return true;
}

void mmd_animation::evaluate(float t, float weight)
{
    auto& world = get_world();

    world.get_view().write<mmd_skeleton_component>().write<mmd_animator_component>().each(
        [=, this](mmd_skeleton_component& skeleton, mmd_animator_component& animator)
        {
            evaluate_motion(skeleton, animator, t, weight);
        });

    world.get_view().write<mmd_skeleton_component>().write<mmd_animator_component>().each(
        [=, this](mmd_skeleton_component& skeleton, mmd_animator_component& animator)
        {
            evaluate_ik(skeleton, animator, t, weight);
        });

    world.get_view()
        .write<morph_component>()
        .write<mmd_animator_component>()
        .read<mesh_component>()
        .each(
            [=, this](
                morph_component& morph,
                mmd_animator_component& animator,
                const mesh_component& mesh)
            {
                evaluate_morph(morph, animator, t);
            });
}

void mmd_animation::update(bool after_physics)
{
    auto& world = get_world();

    world.get_view().write<mmd_skeleton_component>().write<mmd_animator_component>().each(
        [&, this](mmd_skeleton_component& mmd_skeleton, mmd_animator_component& mmd_animator)
        {
            for (std::size_t bone_index : mmd_skeleton.sorted_bones)
            {
                const auto& bone = mmd_skeleton.bones[bone_index];
                auto& motion = mmd_animator.motions[bone_index];
                if (bone.update_after_physics != after_physics)
                {
                    continue;
                }

                update_local(bone, motion);
            }

            for (std::size_t bone_index : mmd_skeleton.sorted_bones)
            {
                auto& bone = mmd_skeleton.bones[bone_index];
                auto& motion = mmd_animator.motions[bone_index];
                if (bone.update_after_physics != after_physics)
                {
                    continue;
                }

                if (bone.inherit_index != -1)
                {
                    update_inherit(
                        bone,
                        motion,
                        mmd_skeleton.bones[bone.inherit_index],
                        mmd_animator.motions[bone.inherit_index]);

                    update_local(bone, motion);
                }

                if (bone.ik_solver)
                {
                    update_ik(mmd_skeleton, mmd_animator, bone, motion);
                }
            }
        });
}

void mmd_animation::evaluate_motion(
    mmd_skeleton_component& skeleton,
    mmd_animator_component& animator,
    float t,
    float weight)
{
    for (auto& bone : skeleton.bones)
    {
        bone.position = bone.initial_position;
        bone.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        bone.inherit_translation = {0.0f, 0.0f, 0.0f};
        bone.inherit_rotation = {0.0f, 0.0f, 0.0f, 1.0f};
        if (bone.ik_link)
        {
            bone.ik_link->rotate = {0.0f, 0.0f, 0.0f, 1.0f};
        }
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

        vec4f_simd translate;
        vec4f_simd rotate;

        if (bound == motion.animation_keys.end())
        {
            translate = math::load(motion.animation_keys.back().translate);
            rotate = math::load(motion.animation_keys.back().rotate);
        }
        else if (bound == motion.animation_keys.begin())
        {
            translate = math::load(bound->translate);
            rotate = math::load(bound->rotate);
        }
        else
        {
            const auto& key0 = *(bound - 1);
            const auto& key1 = *bound;

            float time =
                (t - static_cast<float>(key0.frame)) / static_cast<float>(key1.frame - key0.frame);

            translate = vector::lerp(
                math::load(key0.translate),
                math::load(key1.translate),
                vector::set(
                    key1.tx_bezier.evaluate(time),
                    key1.ty_bezier.evaluate(time),
                    key1.tz_bezier.evaluate(time),
                    0.0f));

            rotate = quaternion::slerp(
                math::load(key0.rotate),
                math::load(key1.rotate),
                key1.r_bezier.evaluate(time));

            motion.offset = std::distance(motion.animation_keys.cbegin(), bound);
        }

        if (weight != 1.0f)
        {
            rotate = quaternion::slerp(math::load(motion.base_rotation), rotate, weight);
            translate = vector::lerp(math::load(motion.base_translation), translate, weight);
        }

        math::store(translate, motion.translation);
        math::store(rotate, motion.rotation);
    }
}

void mmd_animation::evaluate_ik(
    mmd_skeleton_component& skeleton,
    mmd_animator_component& animator,
    float t,
    float weight)
{
    for (std::size_t i = 0; i < skeleton.bones.size(); ++i)
    {
        if (skeleton.bones[i].ik_solver == nullptr)
        {
            continue;
        }

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
            {
                bone.ik_solver->enable = bone.ik_solver->base_animation;
            }
            else
            {
                bone.ik_solver->enable = enable;
            }
        }
    }
}

void mmd_animation::evaluate_morph(
    morph_component& morph,
    mmd_animator_component& animator,
    float t)
{
    for (std::size_t i = 0; i < animator.morphs.size(); ++i)
    {
        auto& animator_morph = animator.morphs[i];

        if (animator_morph.morph_keys.empty())
        {
            continue;
        }

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

            float offset =
                (t - static_cast<float>(key0.frame)) / static_cast<float>(key1.frame - key0.frame);
            weight = key0.weight + (key1.weight - key0.weight) * offset;

            animator_morph.offset = std::distance(animator_morph.morph_keys.cbegin(), bound);
        }

        morph.weights[i] = weight;
    }
}

void mmd_animation::update_inherit(
    mmd_bone& bone,
    mmd_motion& motion,
    mmd_bone& inherit_bone,
    mmd_motion& inherit_motion)
{
    if (bone.is_inherit_rotation)
    {
        vec4f_simd rotate;
        if (!bone.inherit_local_flag && inherit_bone.inherit_index != -1)
        {
            rotate = math::load(inherit_bone.inherit_rotation);
        }
        else
        {
            rotate = math::load(inherit_motion.rotation);
        }

        if (inherit_bone.ik_link)
        {
            rotate = quaternion::mul(math::load(inherit_bone.ik_link->rotate), rotate);
        }

        rotate = quaternion::slerp(quaternion::identity<simd>(), rotate, bone.inherit_weight);
        math::store(rotate, bone.inherit_rotation);
    }

    if (bone.is_inherit_translation)
    {
        vec4f_simd translate;
        if (!bone.inherit_local_flag && inherit_bone.inherit_index != -1)
        {
            translate = math::load(inherit_bone.inherit_translation);
        }
        else
        {
            translate = vector::sub(
                math::load(inherit_bone.position),
                math::load(inherit_bone.initial_position));
        }
        translate = vector::mul(translate, bone.inherit_weight);
        math::store(translate, bone.inherit_translation);
    }
}

void mmd_animation::update_ik(
    mmd_skeleton_component& skeleton,
    mmd_animator_component& animator,
    mmd_bone& bone,
    mmd_motion& motion)
{
    if (bone.ik_solver->target == -1 || !bone.ik_solver->enable)
    {
        return;
    }

    auto& world = get_world();
    auto& transform = get_system<transform_system>();

    for (std::size_t link : bone.ik_solver->links)
    {
        skeleton.bones[link].ik_link->prev_angle = {0.0f, 0.0f, 0.0f};
        skeleton.bones[link].ik_link->rotate = {0.0f, 0.0f, 0.0f, 1.0f};
        skeleton.bones[link].ik_link->plane_mode_angle = 0.0f;
        update_local(skeleton.bones[link], animator.motions[link]);
    }

    float min_dist = std::numeric_limits<float>::max();

    for (std::uint32_t i = 0; i < bone.ik_solver->iteration_count; ++i)
    {
        vec4f_simd ik_position = math::load(transform.get_world_matrix(bone.entity)[3]);
        for (std::uint32_t link : bone.ik_solver->links)
        {
            if (link == bone.ik_solver->target)
            {
                continue;
            }

            auto& link_bone = skeleton.bones[link];
            auto& link_motion = animator.motions[link];

            if (link_bone.ik_link->enable_limit)
            {
                if ((link_bone.ik_link->limit_min.x != 0.0f ||
                     link_bone.ik_link->limit_max.x != 0.0f) &&
                    (link_bone.ik_link->limit_min.y == 0.0f ||
                     link_bone.ik_link->limit_max.y == 0.0f) &&
                    (link_bone.ik_link->limit_min.z == 0.0f ||
                     link_bone.ik_link->limit_max.z == 0.0f))
                {
                    ik_solve_plane(skeleton, animator, bone, link_bone, 0, i);
                    continue;
                }

                if ((link_bone.ik_link->limit_min.x == 0.0f ||
                     link_bone.ik_link->limit_max.x == 0.0f) &&
                    (link_bone.ik_link->limit_min.y != 0.0f ||
                     link_bone.ik_link->limit_max.y != 0.0f) &&
                    (link_bone.ik_link->limit_min.z == 0.0f ||
                     link_bone.ik_link->limit_max.z == 0.0f))
                {
                    ik_solve_plane(skeleton, animator, bone, link_bone, 1, i);
                    continue;
                }

                if ((link_bone.ik_link->limit_min.x == 0.0f ||
                     link_bone.ik_link->limit_max.x == 0.0f) &&
                    (link_bone.ik_link->limit_min.y == 0.0f ||
                     link_bone.ik_link->limit_max.y == 0.0f) &&
                    (link_bone.ik_link->limit_min.z != 0.0f ||
                     link_bone.ik_link->limit_max.z != 0.0f))
                {
                    ik_solve_plane(skeleton, animator, bone, link_bone, 2, i);
                    continue;
                }
            }

            vec4f_simd target_position = math::load(
                transform.get_world_matrix(skeleton.bones[bone.ik_solver->target].entity)[3]);

            mat4f_simd link_inverse = matrix::inverse_transform_without_scale(
                math::load(transform.get_world_matrix(link_bone.entity)));

            vec4f_simd link_ik_position = matrix::mul(ik_position, link_inverse);
            vec4f_simd link_target_position = matrix::mul(target_position, link_inverse);

            vec4f_simd link_ik_vec = vector::normalize(link_ik_position);
            vec4f_simd link_target_vec = vector::normalize(link_target_position);

            float dot = vector::dot(link_ik_vec, link_target_vec);
            dot = math::clamp(dot, -1.0f, 1.0f);

            float angle = std::acos(dot);
            if (angle < 1.0e-3f)
            {
                continue;
            }

            angle = math::clamp(angle, -bone.ik_solver->limit, bone.ik_solver->limit);
            vec4f_simd axis = vector::normalize(vector::cross(link_target_vec, link_ik_vec));

            vec4f_simd link_animate_rotate =
                quaternion::mul(math::load(link_motion.rotation), math::load(link_bone.rotation));

            vec4f_simd link_rotate = math::load(link_bone.ik_link->rotate);
            link_rotate = quaternion::mul(link_rotate, link_animate_rotate);
            link_rotate = quaternion::mul(link_rotate, quaternion::from_axis_angle(axis, angle));

            if (link_bone.ik_link->enable_limit)
            {
                mat4f link_rotate_matrix;
                math::store(matrix::rotation_quaternion(link_rotate), link_rotate_matrix);
                vec3f rotate_xyz = decompose(link_rotate_matrix, link_bone.ik_link->prev_angle);

                vec3f clamp_xyz = vector::clamp(
                    rotate_xyz,
                    link_bone.ik_link->limit_min,
                    link_bone.ik_link->limit_max);
                clamp_xyz = clamp_xyz - link_bone.ik_link->prev_angle;
                clamp_xyz = vector::clamp(
                    clamp_xyz,
                    vec3f{
                        -bone.ik_solver->limit,
                        -bone.ik_solver->limit,
                        -bone.ik_solver->limit,
                    },
                    vec3f{bone.ik_solver->limit, bone.ik_solver->limit, bone.ik_solver->limit});
                clamp_xyz = clamp_xyz + link_bone.ik_link->prev_angle;

                link_rotate = quaternion::from_euler(math::load(clamp_xyz));
                link_bone.ik_link->prev_angle = clamp_xyz;
            }

            link_rotate = quaternion::mul(link_rotate, quaternion::inverse(link_animate_rotate));
            math::store(link_rotate, link_bone.ik_link->rotate);

            update_local(link_bone, animator.motions[link]);
        }

        vec4f_simd target_position = math::load(
            transform.get_world_matrix(skeleton.bones[bone.ik_solver->target].entity)[3]);

        float dist = vector::length(vector::sub(target_position, ik_position));

        if (dist > min_dist)
        {
            for (unsigned int link : bone.ik_solver->links)
            {
                update_local(skeleton.bones[link], animator.motions[link]);
            }
            break;
        }

        min_dist = dist;
    }
}

void mmd_animation::ik_solve_plane(
    mmd_skeleton_component& skeleton,
    mmd_animator_component& animator,
    mmd_bone& bone,
    mmd_bone& ik_link,
    std::uint8_t axis,
    std::size_t iteration)
{
    auto& transform = get_system<transform_system>();

    vec4f_simd rotate_axis;
    switch (axis)
    {
    case 0: // x axis
        rotate_axis = vector::set(1.0f, 0.0f, 0.0f, 0.0f);
        break;
    case 1: // y axis
        rotate_axis = vector::set(0.0f, 1.0f, 0.0f, 0.0f);
        break;
    case 2: // z axis
        rotate_axis = vector::set(0.0f, 0.0f, 1.0f, 0.0f);
        break;
    default:
        return;
    }

    vec4f_simd ik_position = math::load(transform.get_world_matrix(bone.entity)[3]);
    vec4f_simd target_position =
        math::load(transform.get_world_matrix(skeleton.bones[bone.ik_solver->target].entity)[3]);
    mat4f_simd link_inverse = matrix::inverse_transform_without_scale(
        math::load(transform.get_world_matrix(ik_link.entity)));

    vec4f_simd link_ik_vec = matrix::mul(ik_position, link_inverse);
    link_ik_vec = vector::normalize(link_ik_vec);

    vec4f_simd link_target_vec = matrix::mul(target_position, link_inverse);
    link_target_vec = vector::normalize(link_target_vec);

    float dot = vector::dot(link_ik_vec, link_target_vec);
    dot = math::clamp(dot, -1.0f, 1.0f);

    float angle = std::acos(dot);
    angle = math::clamp(angle, -bone.ik_solver->limit, bone.ik_solver->limit);

    vec4f_simd rotate1 = quaternion::from_axis_angle(rotate_axis, angle);
    vec4f_simd target_vec1 = quaternion::mul_vec(rotate1, link_target_vec);
    float dot1 = vector::dot(target_vec1, link_ik_vec);

    vec4f_simd rotate2 = quaternion::from_axis_angle(rotate_axis, -angle);
    vec4f_simd target_vec2 = quaternion::mul_vec(rotate2, link_target_vec);
    float dot2 = vector::dot(target_vec2, link_ik_vec);

    float new_angle = ik_link.ik_link->plane_mode_angle;
    if (dot1 > dot2)
    {
        new_angle += angle;
    }
    else
    {
        new_angle -= angle;
    }

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
                auto half_rad =
                    (ik_link.ik_link->limit_min[axis] + ik_link.ik_link->limit_max[axis]) * 0.5f;
                if (std::abs(half_rad - new_angle) > std::abs(half_rad + new_angle))
                {
                    new_angle *= -1.0f;
                }
            }
        }
    }

    new_angle =
        math::clamp(new_angle, ik_link.ik_link->limit_min[axis], ik_link.ik_link->limit_max[axis]);
    ik_link.ik_link->plane_mode_angle = new_angle;

    vec4f_simd rotation = quaternion::mul(
        math::load(animator.motions[ik_link.index].rotation),
        math::load(skeleton.bones[ik_link.index].rotation));
    vec4f_simd link_rotate = quaternion::from_axis_angle(rotate_axis, new_angle);
    link_rotate = quaternion::mul(link_rotate, quaternion::inverse(rotation));

    math::store(link_rotate, ik_link.ik_link->rotate);

    update_local(skeleton.bones[ik_link.index], animator.motions[ik_link.index]);
}

void mmd_animation::update_local(const mmd_bone& bone, const mmd_motion& motion)
{
    vec4f_simd translate = math::load(bone.position);

    translate = vector::add(math::load(motion.translation), translate);
    if (bone.is_inherit_translation)
    {
        translate = vector::add(translate, math::load(bone.inherit_translation));
    }

    vec4f_simd rotation = math::load(bone.rotation);

    rotation = quaternion::mul(math::load(motion.rotation), rotation);

    if (bone.ik_link)
    {
        rotation = quaternion::mul(math::load(bone.ik_link->rotate), rotation);
    }

    if (bone.is_inherit_rotation)
    {
        rotation = quaternion::mul(rotation, math::load(bone.inherit_rotation));
    }

    auto& transform = get_world().get_component<transform_component>(bone.entity);
    transform.set_position(translate);
    transform.set_rotation(rotation);
}
} // namespace violet
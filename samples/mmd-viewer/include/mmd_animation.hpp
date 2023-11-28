#pragma once

#include "components/mmd_animator.hpp"
#include "components/mmd_morph.hpp"
#include "components/mmd_skeleton.hpp"
#include "components/transform.hpp"
#include "core/engine_system.hpp"
#include "math/math.hpp"

namespace violet::sample
{
class mmd_animation : public engine_system
{
public:
    mmd_animation();

    virtual bool initialize(const dictionary& config) override;

    void evaluate(float t, float weight = 1.0f);
    void update(bool after_physics);

private:
    void evaluate_motion(mmd_skeleton& skeleton, mmd_animator& animator, float t, float weight);
    void evaluate_ik(mmd_skeleton& skeleton, mmd_animator& animator, float t, float weight);
    void evaluate_morph(mmd_morph& morph, mmd_animator& animator, float t);
    void update_inherit(
        mmd_skeleton::bone& bone,
        mmd_animator::motion& motion,
        mmd_skeleton::bone& inherit_bone,
        mmd_animator::motion& inherit_motion);
    void update_ik(
        mmd_skeleton& skeleton,
        mmd_animator& animator,
        mmd_skeleton::bone& bone,
        mmd_animator::motion& motion);
    void ik_solve_core(
        mmd_skeleton& skeleton,
        mmd_animator& animator,
        mmd_skeleton::bone& bone,
        std::size_t iteration);
    void ik_solve_plane(
        mmd_skeleton& skeleton,
        mmd_animator& animator,
        mmd_skeleton::bone& bone,
        mmd_skeleton::bone& ik_link,
        std::uint8_t axis,
        std::size_t iteration);

    void update_local(mmd_skeleton::bone& bone, mmd_animator::motion& motion);

    template <typename Key>
    auto bound_key(const std::vector<Key>& keys, std::int32_t t, std::size_t start)
    {
        if (keys.empty() || keys.size() < start)
            return keys.end();

        return std::upper_bound(
            keys.begin(),
            keys.end(),
            t,
            [](std::int32_t lhs, const Key& rhs)
            {
                return lhs < rhs.frame;
            });
    }
};
} // namespace violet::sample
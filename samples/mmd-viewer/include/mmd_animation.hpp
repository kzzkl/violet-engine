#pragma once

#include "components/mmd_animator_component.hpp"
#include "components/mmd_skeleton_component.hpp"
#include "components/morph_component.hpp"
#include "core/engine_system.hpp"

namespace violet::sample
{
class mmd_animation : public engine_system
{
public:
    mmd_animation();

    bool initialize(const dictionary& config) override;

    void evaluate(float t, float weight = 1.0f);
    void update(bool after_physics);

private:
    void evaluate_motion(
        mmd_skeleton_component& skeleton,
        mmd_animator_component& animator,
        float t,
        float weight);
    void evaluate_ik(
        mmd_skeleton_component& skeleton,
        mmd_animator_component& animator,
        float t,
        float weight);
    void evaluate_morph(morph_component& morph, mmd_animator_component& animator, float t);
    void update_inherit(
        mmd_bone& bone,
        mmd_motion& motion,
        mmd_bone& inherit_bone,
        mmd_motion& inherit_motion);
    void update_ik(
        mmd_skeleton_component& skeleton,
        mmd_animator_component& animator,
        mmd_bone& bone,
        mmd_motion& motion);
    void ik_solve_plane(
        mmd_skeleton_component& skeleton,
        mmd_animator_component& animator,
        mmd_bone& bone,
        mmd_bone& ik_link,
        std::uint8_t axis,
        std::size_t iteration);

    void update_local(const mmd_bone& bone, const mmd_motion& motion);
};
} // namespace violet::sample
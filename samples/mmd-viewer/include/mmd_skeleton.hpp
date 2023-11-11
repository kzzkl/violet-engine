#pragma once

#include "components/transform.hpp"
#include "core/ecs/actor.hpp"
#include "graphics/render_interface.hpp"

namespace violet::sample
{
struct mmd_bone
{
    std::uint32_t index;
    std::int32_t layer;

    bool deform_after_physics;

    bool inherit_local_flag;
    bool is_inherit_rotation;
    bool is_inherit_translation;

    actor* inherit_node;
    float inherit_weight;
    float3 inherit_translation{0.0f, 0.0f, 0.0f};
    float4 inherit_rotation{0.0f, 0.0f, 0.0f, 1.0f};

    float3 initial_position{0.0f, 0.0f, 0.0f};
    float4 initial_rotation{0.0f, 0.0f, 0.0f, 1.0f};
    float3 initial_scale{1.0f, 1.0f, 1.0f};
    float4x4 initial_inverse;
};

class mmd_skeleton
{
public:
    mmd_skeleton(
        rhi_renderer* rhi,
        rhi_parameter_layout* skeleton_layout,
        rhi_parameter_layout* skinning_layout);
    mmd_skeleton(const mmd_skeleton&) = delete;
    mmd_skeleton(mmd_skeleton&& other) noexcept;
    ~mmd_skeleton();

    template <typename BDEF, typename SDEF>
    void set_skinning_data(const std::vector<BDEF>& bdef, const std::vector<SDEF>& sdef)
    {
        if (m_bdef)
            m_rhi->destroy_buffer(m_bdef);
        if (m_sdef)
            m_rhi->destroy_buffer(m_sdef);

        rhi_buffer_desc bdef_desc = {};
        bdef_desc.data = bdef.data();
        bdef_desc.size = bdef.size() * sizeof(BDEF);
        bdef_desc.flags = RHI_BUFFER_FLAG_STORAGE;
        m_bdef = m_rhi->create_buffer(bdef_desc);

        rhi_buffer_desc sdef_desc = {};
        sdef_desc.data = sdef.data();
        sdef_desc.size = sdef.size() * sizeof(SDEF);
        sdef_desc.flags = RHI_BUFFER_FLAG_STORAGE;
        m_sdef = m_rhi->create_buffer(sdef_desc);

        m_skinning_parameter->set_storage(6, m_bdef);
        m_skinning_parameter->set_storage(7, m_sdef);
    }

    void set_skinning_input(
        rhi_resource* positon,
        rhi_resource* normal,
        rhi_resource* uv,
        rhi_resource* skin);

    void set_skinning_output(rhi_resource* positon, rhi_resource* normal, rhi_resource* uv);

    rhi_parameter* get_skeleton_parameter() const noexcept { return m_skeleton_parameter; }
    rhi_parameter* get_skinning_parameter() const noexcept { return m_skinning_parameter; }

    mmd_skeleton& operator=(const mmd_skeleton&) = delete;
    mmd_skeleton& operator=(mmd_skeleton&& other) noexcept;

    std::vector<actor*> bones;
    std::vector<actor*> sorted_bones;

    std::vector<float4x4> local_matrices;
    std::vector<float4x4> world_matrices;

private:
    rhi_parameter* m_skeleton_parameter;
    rhi_parameter* m_skinning_parameter;

    rhi_resource* m_bdef;
    rhi_resource* m_sdef;

    rhi_renderer* m_rhi;
};
using mmd_skeleton_t = mmd_skeleton;
} // namespace violet::sample
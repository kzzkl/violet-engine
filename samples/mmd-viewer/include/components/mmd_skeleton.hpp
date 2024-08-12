#pragma once

#include "components/transform.hpp"
#include "ecs/actor.hpp"
#include "graphics/geometry.hpp"
#include "graphics/render_device.hpp"

namespace violet::sample
{
struct mmd_bone
{
    float4x3 offset;
    float4 quaternion;
};

class mmd_skeleton
{
public:
    struct bone_ik_solver
    {
        bool enable{true};
        float limit;
        std::size_t offset;
        bool base_animation;
        std::size_t target_index;
        std::vector<std::size_t> links;
        std::uint32_t iteration_count;
    };

    struct bone_ik_link
    {
        float4 rotate{0.0f, 0.0f, 0.0f, 1.0f};
        bool enable_limit;
        float3 limit_max;
        float3 limit_min;
        float3 prev_angle;
        float4 save_rotate;
        float plane_mode_angle;
    };

    struct bone
    {
        std::uint32_t index;
        std::int32_t layer;

        bool deform_after_physics;

        bool inherit_local_flag;
        bool is_inherit_rotation;
        bool is_inherit_translation;

        float3 position;
        float4 rotation;
        float3 scale;

        std::size_t inherit_index;
        float inherit_weight;
        float3 inherit_translation{0.0f, 0.0f, 0.0f};
        float4 inherit_rotation{0.0f, 0.0f, 0.0f, 1.0f};

        float3 initial_position{0.0f, 0.0f, 0.0f};
        float4 initial_rotation{0.0f, 0.0f, 0.0f, 1.0f};
        float3 initial_scale{1.0f, 1.0f, 1.0f};
        float4x4 initial_inverse;

        std::unique_ptr<bone_ik_solver> ik_solver;
        std::unique_ptr<bone_ik_link> ik_link;

        component_ptr<transform> transform;
    };

public:
    mmd_skeleton();
    mmd_skeleton(const mmd_skeleton&) = delete;
    mmd_skeleton(mmd_skeleton&& other) = default;
    ~mmd_skeleton();

    template <typename BDEF, typename SDEF>
    void set_skinning_data(const std::vector<BDEF>& bdef, const std::vector<SDEF>& sdef)
    {
        auto& device = render_device::instance();

        m_bdef = nullptr;
        m_sdef = nullptr;

        if (!bdef.empty())
        {
            rhi_buffer_desc bdef_desc = {};
            bdef_desc.data = bdef.data();
            bdef_desc.size = bdef.size() * sizeof(BDEF);
            bdef_desc.flags = RHI_BUFFER_STORAGE;
            m_bdef = device.create_buffer(bdef_desc);
            m_parameter->set_storage(5, m_bdef.get());
        }

        if (!sdef.empty())
        {
            rhi_buffer_desc sdef_desc = {};
            sdef_desc.data = sdef.data();
            sdef_desc.size = sdef.size() * sizeof(SDEF);
            sdef_desc.flags = RHI_BUFFER_STORAGE;
            m_sdef = device.create_buffer(sdef_desc);
            m_parameter->set_storage(6, m_sdef.get());
        }
        else
        {
            SDEF error = {};

            rhi_buffer_desc sdef_desc = {};
            sdef_desc.data = &error;
            sdef_desc.size = sizeof(SDEF);
            sdef_desc.flags = RHI_BUFFER_STORAGE;
            m_sdef = device.create_buffer(sdef_desc);
            m_parameter->set_storage(6, m_sdef.get());
        }
    }

    void set_geometry(geometry* geometry);
    void set_bone(std::size_t index, const mmd_bone& bone);

    rhi_buffer* get_position_buffer() const noexcept { return m_skinned_position.get(); }
    rhi_buffer* get_normal_buffer() const noexcept { return m_skinned_normal.get(); }
    rhi_buffer* get_morph_buffer() const noexcept { return m_morph.get(); }

    rhi_parameter* get_parameter() const noexcept { return m_parameter.get(); }

    mmd_skeleton& operator=(const mmd_skeleton&) = delete;
    mmd_skeleton& operator=(mmd_skeleton&& other) = default;

    std::vector<bone> bones;
    std::vector<std::size_t> sorted_bones;

private:
    rhi_ptr<rhi_parameter> m_parameter;

    rhi_ptr<rhi_buffer> m_bdef;
    rhi_ptr<rhi_buffer> m_sdef;
    rhi_ptr<rhi_buffer> m_skinned_position;
    rhi_ptr<rhi_buffer> m_skinned_normal;
    rhi_ptr<rhi_buffer> m_morph;
};
} // namespace violet::sample
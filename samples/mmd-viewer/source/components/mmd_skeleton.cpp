#include "components/mmd_skeleton.hpp"
#include "mmd_render.hpp"

namespace violet::sample
{
mmd_skeleton::mmd_skeleton(
    rhi_renderer* rhi,
    rhi_parameter_layout* skeleton_layout,
    rhi_parameter_layout* skinning_layout)
    : m_bdef(nullptr),
      m_sdef(nullptr),
      m_rhi(rhi)
{
    m_skeleton_parameter = rhi->create_parameter(skeleton_layout);
    m_skinning_parameter = rhi->create_parameter(skinning_layout);
}

mmd_skeleton::mmd_skeleton(mmd_skeleton&& other) noexcept
{
    *this = std::move(other);
}

mmd_skeleton::~mmd_skeleton()
{
    if (m_rhi != nullptr)
    {
        m_rhi->destroy_parameter(m_skeleton_parameter);
        m_rhi->destroy_parameter(m_skinning_parameter);
    }
}

void mmd_skeleton::set_skinning_input(
    rhi_resource* positon,
    rhi_resource* normal,
    rhi_resource* uv,
    rhi_resource* skin)
{
    m_skinning_parameter->set_storage(0, positon);
    m_skinning_parameter->set_storage(1, normal);
    m_skinning_parameter->set_storage(2, uv);
    m_skinning_parameter->set_storage(8, skin);
}

void mmd_skeleton::set_skinning_output(
    rhi_resource* positon,
    rhi_resource* normal,
    rhi_resource* uv)
{
    m_skinning_parameter->set_storage(3, positon);
    m_skinning_parameter->set_storage(4, normal);
    m_skinning_parameter->set_storage(5, uv);
}

mmd_skeleton& mmd_skeleton::operator=(mmd_skeleton&& other) noexcept
{
    bones = std::move(other.bones);
    sorted_bones = std::move(other.sorted_bones);

    local_matrices = std::move(other.local_matrices);
    world_matrices = std::move(other.world_matrices);

    m_skeleton_parameter = other.m_skeleton_parameter;
    m_skinning_parameter = other.m_skinning_parameter;
    m_rhi = other.m_rhi;

    other.m_skeleton_parameter = nullptr;
    other.m_skinning_parameter = nullptr;
    other.m_rhi = nullptr;

    return *this;
}
} // namespace violet::sample
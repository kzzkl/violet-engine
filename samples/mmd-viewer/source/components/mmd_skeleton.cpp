#include "components/mmd_skeleton.hpp"

namespace violet::sample
{
mmd_skeleton::mmd_skeleton() : m_bdef(nullptr), m_sdef(nullptr)
{
    // m_parameter = m_device->create_parameter(mmd_parameter_layout::skeleton);
}

mmd_skeleton::~mmd_skeleton()
{
}

void mmd_skeleton::set_geometry(geometry* geometry)
{
    auto& device = render_device::instance();

    rhi_buffer_desc skinned_position_desc = {};
    skinned_position_desc.data = nullptr;
    skinned_position_desc.size = sizeof(float3) * geometry->get_vertex_count();
    skinned_position_desc.flags = RHI_BUFFER_VERTEX | RHI_BUFFER_STORAGE;
    m_skinned_position = device.create_buffer(skinned_position_desc);

    rhi_buffer_desc skinned_normal_desc = {};
    skinned_normal_desc.data = nullptr;
    skinned_normal_desc.size = sizeof(float3) * geometry->get_vertex_count();
    skinned_normal_desc.flags = RHI_BUFFER_VERTEX | RHI_BUFFER_STORAGE;
    m_skinned_normal = device.create_buffer(skinned_normal_desc);

    rhi_buffer_desc morph_desc = {};
    morph_desc.data = nullptr;
    morph_desc.size = sizeof(float3) * geometry->get_vertex_count();
    morph_desc.flags = RHI_BUFFER_HOST_VISIBLE | RHI_BUFFER_STORAGE;
    m_morph = device.create_buffer(morph_desc);

    m_parameter->set_storage(0, geometry->get_vertex_buffer("position"));
    m_parameter->set_storage(1, geometry->get_vertex_buffer("normal"));
    m_parameter->set_storage(2, m_skinned_position.get());
    m_parameter->set_storage(3, m_skinned_normal.get());
    m_parameter->set_storage(7, geometry->get_vertex_buffer("skinning type"));
    m_parameter->set_storage(8, m_morph.get());
}

void mmd_skeleton::set_bone(std::size_t index, const mmd_bone& bone)
{
    m_parameter->set_uniform(4, &bone, sizeof(mmd_bone), index * sizeof(mmd_bone));
}
} // namespace violet::sample
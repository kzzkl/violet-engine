#include "components/mmd_skeleton.hpp"
#include "mmd_render.hpp"

namespace violet::sample
{
mmd_skeleton::mmd_skeleton(renderer* renderer)
    : m_bdef(nullptr),
      m_sdef(nullptr),
      m_renderer(renderer)
{

    rhi_parameter_layout* skeleton_layout = renderer->get_parameter_layout("mmd skeleton");
    rhi_parameter_layout* skinning_layout = renderer->get_parameter_layout("mmd skinning");

    m_skeleton_parameter = renderer->create_parameter(skeleton_layout);
    m_skinning_parameter = renderer->create_parameter(skinning_layout);
}

mmd_skeleton::~mmd_skeleton()
{
}

void mmd_skeleton::set_skinning_input(
    rhi_resource* positon,
    rhi_resource* normal,
    rhi_resource* uv,
    rhi_resource* skin,
    rhi_resource* vertex_morph)
{
    m_skinning_parameter->set_storage(0, positon);
    m_skinning_parameter->set_storage(1, normal);
    m_skinning_parameter->set_storage(2, uv);
    m_skinning_parameter->set_storage(8, skin);
    m_skinning_parameter->set_storage(9, vertex_morph);
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
} // namespace violet::sample
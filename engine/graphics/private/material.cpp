#include "graphics/material.hpp"
#include "graphics/material_manager.hpp"

namespace violet
{
material::material(material_type type) noexcept
    : m_type(type)
{
    auto* material_manager = render_device::instance().get_material_manager();
    m_id = material_manager->add_material(this);
}

material::~material()
{
    auto* material_manager = render_device::instance().get_material_manager();
    material_manager->remove_material(m_id);
}

void material::update()
{
    if (!m_dirty)
    {
        return;
    }

    auto* material_manager = render_device::instance().get_material_manager();
    material_manager->update_constant(m_id, get_constant_data(), get_constant_size());

    m_dirty = false;
}

void material::mark_dirty()
{
    assert(get_constant_size() != 0);

    if (m_dirty)
    {
        return;
    }

    m_dirty = true;

    render_device::instance().get_material_manager()->mark_dirty(m_id);
}
} // namespace violet
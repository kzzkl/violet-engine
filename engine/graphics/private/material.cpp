#include "graphics/material.hpp"
#include "graphics/render_context.hpp"
#include "material_manager.hpp"

namespace violet
{
material::material(material_type type, std::size_t constant_size) noexcept
    : m_type(type),
      m_constant_size(constant_size)
{
    auto material_manager = render_context::instance().get_material_manager();
    m_id = material_manager->register_material(this, m_constant_address);
}

material::~material()
{
    auto material_manager = render_context::instance().get_material_manager();
    material_manager->unregister_material(this);
}

void material::mark_dirty()
{
    if (m_dirty)
    {
        return;
    }

    m_dirty = true;

    auto material_manager = render_context::instance().get_material_manager();
    material_manager->mark_dirty(this);
}
} // namespace violet
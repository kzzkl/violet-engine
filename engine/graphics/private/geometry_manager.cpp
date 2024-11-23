#include "geometry_manager.hpp"

namespace violet
{
render_id geometry_manager::register_geometry(geometry* geometry)
{
    render_id geometry_id = m_geometry_allocator.allocate();

    if (m_geometries.size() <= geometry_id)
    {
        m_geometries.resize(geometry_id + 1);
    }

    m_geometries[geometry_id] = geometry;

    return geometry_id;
}

void geometry_manager::unregister_geometry(geometry* geometry)
{
    render_id geometry_id = geometry->get_id();
    m_geometries[geometry_id] = {};
    m_geometry_allocator.free(geometry_id);
}
} // namespace violet
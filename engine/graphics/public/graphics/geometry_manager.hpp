#pragma once

#include "common/allocator.hpp"
#include "graphics/geometry.hpp"

namespace violet
{
class geometry_manager
{
public:
    render_id register_geometry(geometry* geometry);
    void unregister_geometry(render_id geometry_id);

private:
    std::vector<geometry*> m_geometries;
    index_allocator<render_id> m_geometry_allocator;
};
} // namespace violet
#include "graphics/geometry.hpp"
#include "graphics/geometry_manager.hpp"

namespace violet
{
geometry::geometry()
{
    auto* geometry_manager = render_device::instance().get_geometry_manager();
    m_id = geometry_manager->add_geometry(this);
}

geometry::~geometry()
{
    clear_submeshes();

    auto* geometry_manager = render_device::instance().get_geometry_manager();
    geometry_manager->remove_geometry(m_id);
}

void geometry::set_positions(std::span<const vec3f> positions)
{
    set_buffer(
        GEOMETRY_BUFFER_POSITION,
        positions.data(),
        positions.size() * sizeof(vec3f),
        sizeof(vec3f));

    m_vertex_count = static_cast<std::uint32_t>(positions.size());
}

void geometry::set_positions_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_POSITION, src_geometry);
}

std::span<const vec3f> geometry::get_positions() const noexcept
{
    return get_buffer<vec3f>(GEOMETRY_BUFFER_POSITION);
}

void geometry::set_normals(std::span<const vec3f> normals)
{
    set_buffer(
        GEOMETRY_BUFFER_NORMAL,
        normals.data(),
        normals.size() * sizeof(vec3f),
        sizeof(vec3f));
}

void geometry::set_normals_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_NORMAL, src_geometry);
}

std::span<const vec3f> geometry::get_normals() const noexcept
{
    return get_buffer<vec3f>(GEOMETRY_BUFFER_NORMAL);
}

void geometry::set_tangents(std::span<const vec4f> tangents)
{
    set_buffer(
        GEOMETRY_BUFFER_TANGENT,
        tangents.data(),
        tangents.size() * sizeof(vec4f),
        sizeof(vec4f));
}

void geometry::set_tangents_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_TANGENT, src_geometry);
}

std::span<const vec4f> geometry::get_tangents() const noexcept
{
    return get_buffer<vec4f>(GEOMETRY_BUFFER_TANGENT);
}

void geometry::set_texcoords(std::span<const vec2f> texcoord)
{
    set_buffer(
        GEOMETRY_BUFFER_TEXCOORD,
        texcoord.data(),
        texcoord.size() * sizeof(vec2f),
        sizeof(vec2f));
}

void geometry::set_texcoords_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_TEXCOORD, src_geometry);
}

std::span<const vec2f> geometry::get_texcoords() const noexcept
{
    return get_buffer<vec2f>(GEOMETRY_BUFFER_TEXCOORD);
}

void geometry::set_custom_shared(std::size_t index, geometry* src_geometry)
{
    auto type = static_cast<geometry_buffer_type>(GEOMETRY_BUFFER_CUSTOM_0 + index);
    set_buffer_shared(type, src_geometry);
}

void geometry::set_indexes(std::span<const std::uint32_t> indexes)
{
    set_buffer(
        GEOMETRY_BUFFER_INDEX,
        indexes.data(),
        indexes.size() * sizeof(std::uint32_t),
        sizeof(std::uint32_t));

    m_index_count = static_cast<std::uint32_t>(indexes.size());
}

void geometry::set_indexes_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_INDEX, src_geometry);
}

std::span<const std::uint32_t> geometry::get_indexes() const noexcept
{
    return get_buffer<std::uint32_t>(GEOMETRY_BUFFER_INDEX);
}

std::uint32_t geometry::add_submesh(
    std::uint32_t vertex_offset,
    std::uint32_t index_offset,
    std::uint32_t index_count)
{
    auto& submesh = m_submeshes.emplace_back();
    submesh.vertex_offset = vertex_offset;
    submesh.index_offset = index_offset;
    submesh.index_count = index_count;

    if (m_submesh_infos.size() < m_submeshes.size())
    {
        m_submesh_infos.resize(m_submeshes.size());
    }
    m_submesh_infos[m_submeshes.size() - 1].dirty = true;

    mark_dirty();

    return static_cast<std::uint32_t>(m_submeshes.size() - 1);
}

std::uint32_t geometry::add_submesh(
    std::span<const cluster> clusters,
    std::span<const cluster_node> cluster_nodes)
{
    auto& submesh = m_submeshes.emplace_back();
    submesh.clusters.assign(clusters.begin(), clusters.end());
    submesh.cluster_nodes.assign(cluster_nodes.begin(), cluster_nodes.end());

    if (m_submesh_infos.size() < m_submeshes.size())
    {
        m_submesh_infos.resize(m_submeshes.size());
    }
    m_submesh_infos[m_submeshes.size() - 1].dirty = true;

    mark_dirty();

    return static_cast<std::uint32_t>(m_submeshes.size() - 1);
}

void geometry::clear_submeshes()
{
    m_submeshes.clear();
    mark_dirty();
}

void geometry::add_morph_target(std::string_view name, const std::vector<morph_element>& elements)
{
    if (m_morph_target_buffer == nullptr)
    {
        m_morph_target_buffer = std::make_unique<morph_target_buffer>();
    }

    m_morph_name_to_index[std::string(name)] = m_morph_target_buffer->get_morph_target_count();

    m_morph_target_buffer->add_morph_target(elements);
}

void geometry::set_additional_buffer(
    std::string_view name,
    const void* data,
    std::size_t size,
    rhi_buffer_flags flags)
{
    m_additional_buffers[std::string(name)] = std::make_unique<raw_buffer>(data, size, flags);
}

raw_buffer* geometry::get_additional_buffer(std::string_view name) const
{
    auto iter = m_additional_buffers.find(std::string(name));
    return iter == m_additional_buffers.end() ? nullptr : iter->second.get();
}

void geometry::update()
{
    if (!m_dirty)
    {
        return;
    }

    update_buffer();
    update_submesh();

    m_dirty = false;
}

void geometry::set_buffer(
    geometry_buffer_type type,
    const void* data,
    std::size_t size,
    std::size_t stride)
{
    auto& geometry_buffer = m_geometry_buffers[type];

    assert(geometry_buffer.src_geometry == nullptr);

    geometry_buffer.buffer.resize(size);
    std::memcpy(geometry_buffer.buffer.data(), data, size);

    geometry_buffer.stride = static_cast<std::uint32_t>(stride);
    geometry_buffer.dirty = true;

    mark_dirty();
}

void geometry::set_buffer_shared(geometry_buffer_type type, geometry* src_geometry)
{
    auto& geometry_buffer = m_geometry_buffers[type];

    if (geometry_buffer.src_geometry == nullptr)
    {
        geometry_buffer.buffer.clear();
    }

    geometry_buffer.stride = src_geometry->m_geometry_buffers[type].stride;
    geometry_buffer.src_geometry = src_geometry;
    geometry_buffer.dirty = true;

    mark_dirty();
}

void geometry::update_submesh()
{
    auto* geometry_manager = render_device::instance().get_geometry_manager();

    for (std::size_t i = 0; i < m_submeshes.size(); ++i)
    {
        const auto& submesh = m_submeshes[i];
        auto& submesh_info = m_submesh_infos[i];

        if (submesh_info.submesh_id == INVALID_RENDER_ID)
        {
            submesh_info.submesh_id = geometry_manager->add_submesh(m_id);
        }

        if (submesh_info.dirty)
        {
            if (submesh.has_cluster())
            {
                geometry_manager->set_submesh(
                    submesh_info.submesh_id,
                    submesh.clusters,
                    submesh.cluster_nodes);
            }
            else
            {
                geometry_manager->set_submesh(
                    submesh_info.submesh_id,
                    submesh.vertex_offset,
                    submesh.index_offset,
                    submesh.index_count);
            }

            submesh_info.dirty = false;
        }
    }

    while (m_submesh_infos.size() > m_submeshes.size())
    {
        if (m_submesh_infos.back().submesh_id != INVALID_RENDER_ID)
        {
            geometry_manager->remove_submesh(m_submesh_infos.back().submesh_id);
        }

        m_submesh_infos.pop_back();
    }
}

void geometry::update_buffer()
{
    auto* geometry_manager = render_device::instance().get_geometry_manager();

    for (std::size_t i = 0; i < m_geometry_buffers.size(); ++i)
    {
        auto& geometry_buffer = m_geometry_buffers[i];

        if (!geometry_buffer.dirty)
        {
            continue;
        }

        auto type = static_cast<geometry_buffer_type>(i);

        if (geometry_buffer.src_geometry == nullptr)
        {
            geometry_manager->set_buffer(
                m_id,
                type,
                geometry_buffer.buffer.data(),
                geometry_buffer.buffer.size(),
                geometry_buffer.stride);
        }
        else
        {
            geometry_manager->set_shared_buffer(m_id, geometry_buffer.src_geometry->m_id, type);
        }
    }
}

void geometry::mark_dirty()
{
    if (m_dirty)
    {
        return;
    }

    m_dirty = true;

    render_device::instance().get_geometry_manager()->mark_dirty(m_id);
}
} // namespace violet
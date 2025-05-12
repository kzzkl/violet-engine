#include "graphics/geometry.hpp"
#include "graphics/geometry_manager.hpp"
#include "tools/cluster/cluster_builder.hpp"

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
    m_submeshes.emplace_back(vertex_offset, index_offset, index_count);
    m_submesh_infos.push_back({
        .dirty = true,
    });

    mark_dirty();

    return static_cast<std::uint32_t>(m_submeshes.size() - 1);
}

void geometry::set_submesh(
    std::uint32_t submesh_index,
    std::uint32_t vertex_offset,
    std::uint32_t index_offset,
    std::uint32_t index_count)
{
    auto& submesh = m_submeshes[submesh_index];

    if (submesh.vertex_offset == vertex_offset && submesh.index_offset == index_offset &&
        submesh.index_count == index_count)
    {
        return;
    }

    submesh.vertex_offset = vertex_offset;
    submesh.index_offset = index_offset;
    submesh.index_count = index_count;

    m_submesh_infos[submesh_index].dirty = true;

    mark_dirty();
}

void geometry::clear_submeshes()
{
    m_submeshes.clear();
    mark_dirty();
}

void geometry::generate_clusters()
{
    auto old_positions = get_positions();
    auto old_indexes = get_indexes();

    std::vector<vec3f> new_positions;
    std::vector<std::uint32_t> new_indexes;

    for (std::size_t submesh_index = 0; submesh_index < m_submeshes.size(); ++submesh_index)
    {
        const auto& submesh = m_submeshes[submesh_index];

        std::unordered_map<std::uint32_t, std::uint32_t> vertex_remap;

        std::vector<vec3f> positions;
        std::vector<std::uint32_t> indexes;
        indexes.reserve(submesh.index_count);

        for (std::uint32_t i = 0; i < submesh.index_count; ++i)
        {
            std::uint32_t vertex_index =
                old_indexes[submesh.index_offset + i] + submesh.vertex_offset;

            auto iter = vertex_remap.find(vertex_index);
            if (iter == vertex_remap.end())
            {
                positions.push_back(old_positions[vertex_index]);
                vertex_remap[vertex_index] = static_cast<std::uint32_t>(positions.size() - 1);
                vertex_index = static_cast<std::uint32_t>(positions.size() - 1);
            }
            else
            {
                vertex_index = iter->second;
            }

            indexes.push_back(vertex_index);
        }

        cluster_builder builder;
        builder.set_positions(positions);
        builder.set_indexes(indexes);

        builder.build();

        auto vertex_offset = static_cast<std::uint32_t>(new_positions.size());
        auto index_offset = static_cast<std::uint32_t>(new_indexes.size());

        new_positions.insert(
            new_positions.end(),
            builder.get_positions().begin(),
            builder.get_positions().end());
        new_indexes.insert(
            new_indexes.end(),
            builder.get_indexes().begin(),
            builder.get_indexes().end());
        for (std::uint32_t i = index_offset; i < new_indexes.size(); ++i)
        {
            new_indexes[i] += vertex_offset;
        }

        auto& submesh_info = m_submesh_infos[submesh_index];
        submesh_info.cluster_offset = static_cast<std::uint32_t>(m_clusters.size());
        submesh_info.cluster_count = static_cast<std::uint32_t>(builder.get_clusters().size());

        const auto& groups = builder.get_groups();
        const auto& clusters = builder.get_clusters();

        for (const auto& group : groups)
        {
            for (std::uint32_t i = 0; i < group.cluster_count; ++i)
            {
                const auto& cluster = clusters[group.cluster_offset + i];

                m_clusters.push_back({
                    .index_offset = cluster.index_offset + index_offset,
                    .index_count = cluster.index_count,
                    .bounding_sphere = cluster.bounding_sphere,
                    .lod_bounds = cluster.lod_bounds,
                    .lod_error = cluster.lod_error,
                    .parent_lod_bounds = cluster.parent_lod_bounds,
                    .parent_lod_error = cluster.parent_lod_error,
                    .lod = group.lod,
                    .children_offset = groups[cluster.children_group].cluster_offset,
                    .children_count = groups[cluster.children_group].cluster_count,
                });
            }
        }
    }

    set_positions(new_positions);
    set_indexes(new_indexes);
}

void geometry::add_morph_target(std::string_view name, const std::vector<morph_element>& elements)
{
    if (m_morph_target_buffer == nullptr)
    {
        m_morph_target_buffer = std::make_unique<morph_target_buffer>();
    }

    m_morph_name_to_index[name.data()] = m_morph_target_buffer->get_morph_target_count();

    m_morph_target_buffer->add_morph_target(elements);
}

void geometry::set_additional_buffer(
    std::string_view name,
    const void* data,
    std::size_t size,
    rhi_buffer_flags flags)
{
    m_additional_buffers[name.data()] = std::make_unique<raw_buffer>(data, size, flags);
}

raw_buffer* geometry::get_additional_buffer(std::string_view name) const
{
    auto iter = m_additional_buffers.find(name.data());
    return iter == m_additional_buffers.end() ? nullptr : iter->second.get();
}

void geometry::update()
{
    if (!m_dirty)
    {
        return;
    }

    update_cluster();
    update_submesh();
    update_buffer();

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

void geometry::update_cluster() {}

void geometry::update_submesh()
{
    auto* geometry_manager = render_device::instance().get_geometry_manager();

    for (std::size_t i = 0; i < m_submeshes.size(); ++i)
    {
        if (m_submesh_infos[i].submesh_id == INVALID_RENDER_ID)
        {
            m_submesh_infos[i].submesh_id = geometry_manager->add_submesh(m_id);
        }

        if (m_submesh_infos[i].dirty)
        {
            geometry_manager->set_submesh(
                m_submesh_infos[i].submesh_id,
                m_submeshes[i].vertex_offset,
                m_submeshes[i].index_offset,
                m_submeshes[i].index_count);

            m_submesh_infos[i].dirty = false;
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
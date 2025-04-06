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
    auto* geometry_manager = render_device::instance().get_geometry_manager();
    geometry_manager->remove_geometry(m_id);
}

void geometry::set_position(std::span<const vec3f> position)
{
    if (m_vertex_capacity == 0)
    {
        m_vertex_capacity = position.size();
    }

    // TODO: If the vertex buffer cannot accommodate all vertices, a new vertex buffer needs to
    // be reallocated, and the vertex offset in shader::mesh_data must be re-linked. However,
    // there is currently no reliable method to re-establish this linkage, so we will simply
    // reject this operation for now.
    assert(m_vertex_capacity >= position.size());

    set_buffer(
        GEOMETRY_BUFFER_POSITION,
        position.data(),
        position.size() * sizeof(vec3f),
        sizeof(vec3f));

    m_vertex_count = static_cast<std::uint32_t>(position.size());

    m_aabb = {};
    for (const auto& position : position)
    {
        box::expand(m_aabb, position);
    }
}

void geometry::set_position_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_POSITION, src_geometry);
}

std::span<const vec3f> geometry::get_position() const noexcept
{
    return get_buffer<vec3f>(GEOMETRY_BUFFER_POSITION);
}

void geometry::set_normal(std::span<const vec3f> normal)
{
    set_buffer(GEOMETRY_BUFFER_NORMAL, normal.data(), normal.size() * sizeof(vec3f), sizeof(vec3f));
}

void geometry::set_normal_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_NORMAL, src_geometry);
}

std::span<const vec3f> geometry::get_normal() const noexcept
{
    return get_buffer<vec3f>(GEOMETRY_BUFFER_NORMAL);
}

void geometry::set_tangent(std::span<const vec4f> tangent)
{
    set_buffer(
        GEOMETRY_BUFFER_TANGENT,
        tangent.data(),
        tangent.size() * sizeof(vec4f),
        sizeof(vec4f));
}

void geometry::set_tangent_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_TANGENT, src_geometry);
}

std::span<const vec4f> geometry::get_tangent() const noexcept
{
    return get_buffer<vec4f>(GEOMETRY_BUFFER_TANGENT);
}

void geometry::set_texcoord(std::span<const vec2f> texcoord)
{
    set_buffer(
        GEOMETRY_BUFFER_TEXCOORD,
        texcoord.data(),
        texcoord.size() * sizeof(vec2f),
        sizeof(vec2f));
}

void geometry::set_texcoord_shared(geometry* src_geometry)
{
    set_buffer_shared(GEOMETRY_BUFFER_TEXCOORD, src_geometry);
}

std::span<const vec2f> geometry::get_texcoord() const noexcept
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
                geometry_buffer.buffer.size());
        }
        else
        {
            geometry_manager->set_shared_buffer(m_id, geometry_buffer.src_geometry->get_id(), type);
        }
    }

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

    geometry_buffer.stride = stride;
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
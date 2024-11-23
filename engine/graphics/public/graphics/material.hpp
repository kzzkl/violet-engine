#pragma once

#include "common/allocator.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"

namespace violet
{
enum material_type
{
    MATERIAL_OPAQUE,
    MATERIAL_TRANSPARENT,
    MATERIAL_OUTLINE,
};

enum lighting_type
{
    LIGHTING_UNLIT = 1,
    LIGHTING_PHYSICAL = 2,
};

class material
{
public:
    struct pass_info
    {
        rdg_render_pipeline pipeline;
        rhi_ptr<rhi_parameter> parameter;
    };

public:
    material(material_type type, std::size_t constant_size = 0) noexcept;
    material(const material& other) = delete;

    virtual ~material();

    material& operator=(const material& other) = delete;

    const rdg_render_pipeline& get_pipeline() const noexcept
    {
        return m_pipeline;
    }

    material_type get_type() const noexcept
    {
        return m_type;
    }

    render_id get_id() const noexcept
    {
        return m_id;
    }

    virtual const void* get_constant_data() const noexcept
    {
        return nullptr;
    }

    std::size_t get_constant_size() const noexcept
    {
        return m_constant_size;
    }

    std::uint32_t get_constant_address() const noexcept
    {
        return m_constant_address;
    }

    void clear_dirty() noexcept
    {
        m_dirty = false;
    }

protected:
    void set_pipeline(const rdg_render_pipeline& pipeline) noexcept
    {
        m_pipeline = pipeline;
    }

    void mark_dirty();

private:
    material_type m_type;
    rdg_render_pipeline m_pipeline;

    render_id m_id{INVALID_RENDER_ID};

    std::size_t m_constant_size{0};
    std::uint32_t m_constant_address{0};

    bool m_dirty{false};
};

template <typename T>
class mesh_material : public material
{
public:
    using constant_type = T;

public:
    mesh_material(material_type type)
        : material(type, sizeof(constant_type))
    {
    }

    virtual const void* get_constant_data() const noexcept
    {
        return &m_constant;
    }

protected:
    constant_type& get_constant() noexcept
    {
        mark_dirty();
        return m_constant;
    }

    const constant_type& get_constant() const noexcept
    {
        return m_constant;
    }

private:
    constant_type m_constant{};
};
} // namespace violet
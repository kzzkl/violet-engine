#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"

namespace violet
{
using material_type = std::uint32_t;

enum buildin_material_type : material_type
{
    MATERIAL_OPAQUE,
    MATERIAL_TRANSPARENT,
    MATERIAL_CUSTOM,
};

using shading_model = std::uint32_t;

enum buildin_shading_model : shading_model
{
    SHADING_MODEL_UNLIT = 1,
    SHADING_MODEL_PHYSICAL,
    SHADING_MODEL_CUSTOM,
};

class material
{
public:
    struct pass_info
    {
        rdg_raster_pipeline pipeline;
        rhi_ptr<rhi_parameter> parameter;
    };

    material(material_type type, std::size_t constant_size = 0) noexcept;
    material(const material& other) = delete;

    virtual ~material();

    material& operator=(const material& other) = delete;

    const rdg_raster_pipeline& get_pipeline() const noexcept
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
    rdg_raster_pipeline& get_pipeline() noexcept
    {
        return m_pipeline;
    }

    void mark_dirty();

private:
    material_type m_type;
    rdg_raster_pipeline m_pipeline{};

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

    mesh_material(material_type type)
        : material(type, sizeof(constant_type))
    {
    }

    const void* get_constant_data() const noexcept override
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
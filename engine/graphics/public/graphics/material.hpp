#pragma once

#include "graphics/render_device.hpp"
#include "graphics/shading_model.hpp"
#include <functional>

namespace violet
{
using surface_type = std::uint8_t;

enum buildin_surface_type : std::uint8_t
{
    SURFACE_TYPE_OPAQUE,
    SURFACE_TYPE_TRANSPARENT,
    SURFACE_TYPE_CUSTOM,
};

enum material_path : std::uint8_t
{
    MATERIAL_PATH_FORWARD,
    MATERIAL_PATH_DEFERRED,
    MATERIAL_PATH_VISIBILITY,
};

class material
{
public:
    material() noexcept;
    material(const material& other) = delete;

    virtual ~material();

    material& operator=(const material& other) = delete;

    surface_type get_surface_type() const noexcept
    {
        return m_surface_type;
    }

    const rdg_raster_pipeline& get_pipeline() const noexcept
    {
        return m_pipeline;
    }

    virtual material_path get_material_path() const noexcept = 0;

    render_id get_material_id() const noexcept
    {
        return m_material_id;
    }

    std::pair<std::uint32_t, std::uint32_t> get_material_info() const noexcept
    {
        return {(m_material_info & 0xFFFFFF00) >> 8, m_material_info & 0x000000FF};
    }

    void update();

protected:
    void set_surface_type(surface_type surface_type) noexcept
    {
        m_surface_type = surface_type;
    }

    std::uint32_t set_pipeline_impl(const rdg_raster_pipeline& pipeline);
    std::uint32_t set_pipeline_impl(
        const rdg_raster_pipeline& pipeline,
        render_id shading_model_id,
        const std::function<std::unique_ptr<shading_model_base>()>& creator);
    std::uint32_t set_pipeline_impl(
        const rdg_raster_pipeline& visibility_pipeline,
        const rdg_compute_pipeline& material_resolve_pipeline,
        render_id shading_model_id,
        const std::function<std::unique_ptr<shading_model_base>()>& creator);

    void mark_dirty();

private:
    virtual std::pair<const void*, std::size_t> get_constant_data() noexcept
    {
        return {};
    }

    surface_type m_surface_type;

    rdg_raster_pipeline m_pipeline{};

    render_id m_material_id{INVALID_RENDER_ID};

    // 24 bit: material resolve pipeline id, 8 bit: shading model id
    std::uint32_t m_material_info{0};

    bool m_dirty{false};
};

struct shading_model_index : public type_index<shading_model_index, std::uint32_t, 1>
{
};

template <typename Constant, material_path Path>
class mesh_material : public material
{
public:
    using constant_type = Constant;

    material_path get_material_path() const noexcept final
    {
        return Path;
    }

protected:
    constant_type& get_constant() noexcept
    {
        mark_dirty();
        return m_wrapper.constant;
    }

    const constant_type& get_constant() const noexcept
    {
        return m_wrapper.constant;
    }

    void set_pipeline(const rdg_raster_pipeline& pipeline)
        requires(Path == MATERIAL_PATH_FORWARD)
    {
        set_pipeline_impl(pipeline);
    }

    template <typename ShadingModel>
    void set_pipeline(const rdg_raster_pipeline& pipeline)
        requires(Path == MATERIAL_PATH_DEFERRED)
    {
        m_wrapper.material_info = set_pipeline_impl(
            pipeline,
            shading_model_index::value<ShadingModel>(),
            []()
            {
                return std::make_unique<ShadingModel>();
            });
    }

    template <typename ShadingModel>
    void set_pipeline(
        const rdg_raster_pipeline& visibility_pipeline,
        const rdg_compute_pipeline& material_resolve_pipeline)
        requires(Path == MATERIAL_PATH_VISIBILITY)
    {
        m_wrapper.material_info = set_pipeline_impl(
            visibility_pipeline,
            material_resolve_pipeline,
            shading_model_index::value<ShadingModel>(),
            []()
            {
                return std::make_unique<ShadingModel>();
            });
    }

    template <typename T>
    void set_shading_model() noexcept
        requires(Path == MATERIAL_PATH_DEFERRED || Path == MATERIAL_PATH_VISIBILITY)
    {
        render_id shading_model_id = shading_model_index::value<T>();

        set_shading_model(
            shading_model_id,
            []()
            {
                return std::make_unique<T>();
            });

        m_wrapper.material_info = shading_model_id | (m_wrapper.material_info & 0xFFFFFF00);
    }

private:
    struct wrapper
    {
        std::uint32_t material_info;
        constant_type constant;
    };

    std::pair<const void*, std::size_t> get_constant_data() noexcept override
    {
        return {&m_wrapper, sizeof(wrapper)};
    }

    using material::set_pipeline_impl;
    using material::mark_dirty;

    wrapper m_wrapper{};
};

struct visibility_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/visibility/visibility.hlsl";
};

struct visibility_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/visibility/visibility.hlsl";
};
} // namespace violet
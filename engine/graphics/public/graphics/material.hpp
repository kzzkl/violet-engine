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

    std::uint32_t get_resolve_pipeline() const noexcept
    {
        return m_resolve_pipeline;
    }

    std::uint32_t get_shading_model() const noexcept
    {
        return m_shading_model;
    }

    void update();

protected:
    void set_surface_type(surface_type surface_type) noexcept
    {
        m_surface_type = surface_type;
    }

    void set_pipeline_impl(const rdg_raster_pipeline& pipeline);
    void set_pipeline_impl(
        const rdg_raster_pipeline& pipeline,
        render_id shading_model_id,
        const std::function<std::unique_ptr<shading_model_base>()>& creator);
    void set_pipeline_impl(
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

    std::uint32_t m_resolve_pipeline{0};
    std::uint32_t m_shading_model{0};

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

    mesh_material()
    {
        set_opacity_cutoff(0.5f);
    }

    material_path get_material_path() const noexcept final
    {
        return Path;
    }

    void set_opacity_cutoff(float opacity_cutoff)
    {
        m_wrapper.material_info.y &= 0xFFFFFF00;
        m_wrapper.material_info.y |= static_cast<std::uint32_t>(opacity_cutoff * 255);
        mark_dirty();
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
        set_pipeline_impl(
            pipeline,
            shading_model_index::value<ShadingModel>(),
            []()
            {
                return std::make_unique<ShadingModel>();
            });

        std::uint32_t resolve_pipeline = get_resolve_pipeline();
        std::uint32_t shading_model = get_shading_model();
        m_wrapper.material_info.x = resolve_pipeline << 8;
        m_wrapper.material_info.x |= shading_model;

        mark_dirty();
    }

    template <typename ShadingModel>
    void set_pipeline(
        const rdg_raster_pipeline& visibility_pipeline,
        const rdg_compute_pipeline& material_resolve_pipeline)
        requires(Path == MATERIAL_PATH_VISIBILITY)
    {
        set_pipeline_impl(
            visibility_pipeline,
            material_resolve_pipeline,
            shading_model_index::value<ShadingModel>(),
            []()
            {
                return std::make_unique<ShadingModel>();
            });

        std::uint32_t resolve_pipeline = get_resolve_pipeline();
        std::uint32_t shading_model = get_shading_model();
        m_wrapper.material_info.x = resolve_pipeline << 8;
        m_wrapper.material_info.x |= shading_model;

        mark_dirty();
    }

    void set_opacity_mask(std::uint32_t opacity_mask)
    {
        m_wrapper.material_info.y &= 0x000000FF;
        m_wrapper.material_info.y |= opacity_mask << 8;
        mark_dirty();
    }

private:
    struct wrapper
    {
        vec2u material_info;
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
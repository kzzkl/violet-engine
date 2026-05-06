#pragma once

#include "graphics/render_device.hpp"
#include "graphics/shading_model.hpp"
#include <functional>

namespace violet
{
struct visibility_vs : public mesh_vs
{
    static constexpr std::string_view path = "assets/shaders/visibility/visibility.hlsl";
};

struct visibility_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/visibility/visibility.hlsl";
};

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

enum shadow_cull_mode : std::uint8_t
{
    SHADOW_CULL_MODE_AUTO,
    SHADOW_CULL_MODE_NONE,
    SHADOW_CULL_MODE_BACK,
    SHADOW_CULL_MODE_FRONT,
};

class material
{
public:
    enum dirty_flag : std::uint8_t
    {
        DIRTY_FLAG_CONSTANT = 1 << 0,
        DIRTY_FLAG_PIPELINE = 1 << 1,
        DIRTY_FLAG_SHADOW_CULL_MODE = 1 << 2,
        DIRTY_FLAG_ALL = 0xFF,
    };
    using dirty_flags = std::uint8_t;

    material() noexcept;
    material(const material& other) = delete;

    virtual ~material();

    material& operator=(const material& other) = delete;

    surface_type get_surface_type() const noexcept
    {
        return m_surface_type;
    }

    const rdg_raster_pipeline& get_raster_pipeline() const noexcept
    {
        return m_raster_pipeline;
    }

    virtual material_path get_material_path() const noexcept = 0;

    render_id get_material_id() const noexcept
    {
        return m_material_id;
    }

    std::uint32_t get_resolve_pipeline() const noexcept
    {
        return m_resolve_pipeline_id;
    }

    std::uint32_t get_shading_model() const noexcept
    {
        return m_shading_model;
    }

    virtual bool get_opacity_cutoff() const noexcept
    {
        return false;
    }

    void set_cull_mode(rhi_cull_mode cull_mode);
    void set_polygon_mode(rhi_polygon_mode polygon_mode);
    void set_primitive_topology(rhi_primitive_topology primitive_topology);

    void set_shadow_cull_mode(shadow_cull_mode cull_mode);
    shadow_cull_mode get_shadow_cull_mode() const noexcept
    {
        return m_shadow_cull_mode;
    }

    std::uint32_t get_shadow_batch() const noexcept
    {
        return m_shadow_batch;
    }

    void update();

    dirty_flags get_dirty_flags() const noexcept
    {
        return m_dirty_flags;
    }

protected:
    void set_surface_type(surface_type surface_type) noexcept
    {
        m_surface_type = surface_type;
    }

    void set_shading_model_impl(
        render_id shading_model_id,
        const std::function<std::unique_ptr<shading_model_base>()>& creator);

    virtual rhi_shader* get_vertex_shader(std::span<std::wstring> defines) const = 0;
    virtual rhi_shader* get_geometry_shader(std::span<std::wstring> defines) const = 0;
    virtual rhi_shader* get_fragment_shader(std::span<std::wstring> defines) const = 0;
    virtual rhi_shader* get_resolve_shader(std::span<std::wstring> defines) const = 0;

    void mark_dirty(dirty_flags dirty_flags);

private:
    virtual std::pair<const void*, std::size_t> get_constant_data(
        std::uint32_t shading_model,
        std::uint32_t resolve_pipeline,
        std::uint32_t shadow_batch) noexcept
    {
        return {};
    }

    surface_type m_surface_type;

    rdg_raster_pipeline m_raster_pipeline{};
    rdg_compute_pipeline m_resolve_pipeline{};

    render_id m_material_id{INVALID_RENDER_ID};

    std::uint32_t m_resolve_pipeline_id{0};
    std::uint32_t m_shading_model{0};

    shadow_cull_mode m_shadow_cull_mode{SHADOW_CULL_MODE_AUTO};
    std::uint32_t m_shadow_batch{0};

    dirty_flags m_dirty_flags{0};
};

struct shading_model_index : public type_index<shading_model_index, std::uint32_t, 1>
{
};

template <material_path Path>
class material_variant : public material
{
};

template <>
class material_variant<MATERIAL_PATH_FORWARD> : public material
{
public:
    material_path get_material_path() const noexcept final
    {
        return MATERIAL_PATH_FORWARD;
    }

private:
    rhi_shader* get_geometry_shader(std::span<std::wstring> defines) const override
    {
        return nullptr;
    }

    rhi_shader* get_resolve_shader(std::span<std::wstring> defines) const override
    {
        return nullptr;
    }

    using material::set_shading_model_impl;
};

template <>
class material_variant<MATERIAL_PATH_DEFERRED> : public material
{
public:
    material_path get_material_path() const noexcept final
    {
        return MATERIAL_PATH_DEFERRED;
    }

protected:
    template <typename ShadingModel>
    void set_shading_model()
    {
        set_shading_model_impl(
            shading_model_index::value<ShadingModel>(),
            []()
            {
                return std::make_unique<ShadingModel>();
            });
    }

private:
    rhi_shader* get_geometry_shader(std::span<std::wstring> defines) const override
    {
        return nullptr;
    }

    rhi_shader* get_resolve_shader(std::span<std::wstring> defines) const override
    {
        return nullptr;
    }

    using material::set_shading_model_impl;
};

template <>
class material_variant<MATERIAL_PATH_VISIBILITY> : public material
{
public:
    material_path get_material_path() const noexcept final
    {
        return MATERIAL_PATH_VISIBILITY;
    }

protected:
    template <typename ShadingModel>
    void set_shading_model()
    {
        set_shading_model_impl(
            shading_model_index::value<ShadingModel>(),
            []()
            {
                return std::make_unique<ShadingModel>();
            });
    }

private:
    rhi_shader* get_vertex_shader(std::span<std::wstring> defines) const override
    {
        return render_device::instance().get_shader<visibility_vs>(defines);
    }

    rhi_shader* get_geometry_shader(std::span<std::wstring> defines) const override
    {
        return nullptr;
    }

    rhi_shader* get_fragment_shader(std::span<std::wstring> defines) const override
    {
        return render_device::instance().get_shader<visibility_fs>(defines);
    }

    using material::set_shading_model_impl;
};

template <typename Constant, material_path Path>
class material_instance : public material_variant<Path>
{
public:
    using constant_type = Constant;

    material_instance() = default;

    void set_opacity_cutoff(float opacity_cutoff)
    {
        m_wrapper.material_info.y &= 0xFFFFFF00;
        m_wrapper.material_info.y |= static_cast<std::uint32_t>(opacity_cutoff * 255);
        material::mark_dirty(material::DIRTY_FLAG_CONSTANT);
    }

    bool get_opacity_cutoff() const noexcept override
    {
        return (m_wrapper.material_info.y & 0xFF) != 0;
    }

protected:
    constant_type& get_constant() noexcept
    {
        material::mark_dirty(material::DIRTY_FLAG_CONSTANT);
        return m_wrapper.constant;
    }

    const constant_type& get_constant() const noexcept
    {
        return m_wrapper.constant;
    }

    void set_opacity_mask(std::uint32_t opacity_mask)
    {
        m_wrapper.material_info.y &= 0x000000FF;
        m_wrapper.material_info.y |= opacity_mask << 8;
        material::mark_dirty(material::DIRTY_FLAG_CONSTANT);
    }

private:
    struct wrapper
    {
        vec2u material_info;
        constant_type constant;
    };

    std::pair<const void*, std::size_t> get_constant_data(
        std::uint32_t shading_model,
        std::uint32_t resolve_pipeline,
        std::uint32_t shadow_batch) noexcept override
    {
        m_wrapper.material_info.x = (resolve_pipeline << 16) | (shading_model << 8) | shadow_batch;
        return {&m_wrapper, sizeof(wrapper)};
    }

    wrapper m_wrapper{};
};
} // namespace violet
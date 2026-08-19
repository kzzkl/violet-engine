#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_pipeline.hpp"
#include "graphics/shading_model.hpp"
#include <algorithm>
#include <cassert>
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
    MATERIAL_PATH_AUTO,
    MATERIAL_PATH_FORWARD,
    MATERIAL_PATH_DEFERRED,
    MATERIAL_PATH_VISIBILITY,
};

enum material_flag : std::uint8_t
{
    MATERIAL_FLAG_NONE = 0,
    MATERIAL_FLAG_OPACITY_CUTOFF = 1 << 0,
    MATERIAL_FLAG_WORLD_POSITION_OFFSET = 1 << 1,
};
using material_flags = std::uint8_t;

enum shadow_cull_mode : std::uint8_t
{
    SHADOW_CULL_MODE_AUTO,
    SHADOW_CULL_MODE_NONE,
    SHADOW_CULL_MODE_BACK,
    SHADOW_CULL_MODE_FRONT,
};

struct shading_model_index : public type_index<shading_model_index, std::uint32_t, 1>
{
};

class material
{
public:
    class header
    {
    public:
        void set_shading_model(render_id shading_model_id)
        {
            m_data.x = (m_data.x & ~shading_model_mask) | (shading_model_id & shading_model_mask);
        }

        render_id get_shading_model() const
        {
            return m_data.x & shading_model_mask;
        }

        void set_shadow_cull_mode(rhi_cull_mode cull_mode)
        {
            m_data.x = (m_data.x & ~shadow_cull_mode_mask) |
                       (static_cast<std::uint32_t>(cull_mode) << shadow_cull_mode_shift);
        }

        rhi_cull_mode get_shadow_cull_mode() const
        {
            return static_cast<rhi_cull_mode>(
                (m_data.x & shadow_cull_mode_mask) >> shadow_cull_mode_shift);
        }

        void set_opacity_cutoff(float opacity_cutoff)
        {
            auto value = static_cast<std::uint32_t>(opacity_cutoff * 255.0f);
            value = std::min(value, 0xFFu);

            m_data.y = (m_data.y & ~opacity_cutoff_mask) | value;
        }

        float get_opacity_cutoff() const
        {
            return static_cast<float>(m_data.y & opacity_cutoff_mask) / 255.0f;
        }

        void set_opacity_mask(std::uint32_t opacity_mask)
        {
            m_data.y = (m_data.y & opacity_cutoff_mask) | (opacity_mask << opacity_mask_shift);
        }

        std::uint32_t get_opacity_mask() const
        {
            return (m_data.y & opacity_mask_mask) >> opacity_mask_shift;
        }

        void set_resolve_pipeline(render_id resolve_pipeline_id)
        {
            m_data.z = resolve_pipeline_id;
        }

        render_id get_resolve_pipeline() const
        {
            return m_data.z;
        }

    private:
        static constexpr std::uint32_t shading_model_mask = 0x000000FF;
        static constexpr std::uint32_t shadow_cull_mode_shift = 8;
        static constexpr std::uint32_t shadow_cull_mode_mask = 0x00000F00;
        static constexpr std::uint32_t opacity_cutoff_mask = 0x000000FF;
        static constexpr std::uint32_t opacity_mask_shift = 8;
        static constexpr std::uint32_t opacity_mask_mask = 0xFFFFFF00;

        vec4u m_data;
    };

    enum dirty_flag : std::uint8_t
    {
        DIRTY_FLAG_CONSTANT = 1 << 0,
        DIRTY_FLAG_PIPELINE = 1 << 1,
        DIRTY_FLAG_SHADOW_CULL_MODE = 1 << 2,
        DIRTY_FLAG_ALL = 0xFF,
    };
    using dirty_flags = std::uint8_t;

    material(std::string_view name, std::size_t constant_size, std::string_view shader_path = "");
    material(const material& other) = delete;

    virtual ~material();

    material& operator=(const material& other) = delete;

    material_path get_material_path() const
    {
        if (m_material_path != MATERIAL_PATH_AUTO)
        {
            return m_material_path;
        }

        return MATERIAL_PATH_VISIBILITY;
    }

    surface_type get_surface_type() const noexcept
    {
        return m_surface_type;
    }

    const rdg_raster_pipeline& get_raster_pipeline() const noexcept
    {
        return m_raster_pipeline;
    }

    render_id get_material_id() const
    {
        return m_material_id;
    }

    render_id get_raster_pipeline_id() const
    {
        return m_raster_pipeline_id;
    }

    render_id get_resolve_pipeline_id() const
    {
        return get_header().get_resolve_pipeline();
    }

    render_id get_shading_model_id() const
    {
        return get_header().get_shading_model();
    }

    bool get_opacity_cutoff() const
    {
        return get_header().get_opacity_cutoff() != 0;
    }

    void set_opacity_cutoff(float opacity_cutoff)
    {
        get_header().set_opacity_cutoff(opacity_cutoff);
    }

    void set_opacity_mask(std::uint32_t opacity_mask)
    {
        get_header().set_opacity_mask(opacity_mask);
    }

    void set_cull_mode(rhi_cull_mode cull_mode);
    void set_polygon_mode(rhi_polygon_mode polygon_mode);
    void set_primitive_topology(rhi_primitive_topology primitive_topology);

    void set_blend_state(const rhi_blend_state* blend_state);

    void set_shadow_cull_mode(shadow_cull_mode cull_mode);
    shadow_cull_mode get_shadow_cull_mode() const noexcept
    {
        return m_shadow_cull_mode;
    }

    std::uint32_t get_shadow_batch() const noexcept
    {
        const auto& header = get_header();
        return header.get_shadow_cull_mode() << 1 | (header.get_opacity_cutoff() == 0.0f ? 0 : 1);
    }

    void update();

    dirty_flags get_dirty_flags() const noexcept
    {
        return m_dirty_flags;
    }

protected:
    void set_material_path(material_path path)
    {
        m_material_path = path;
    }

    void set_surface_type(surface_type surface_type) noexcept
    {
        m_surface_type = surface_type;
    }

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

    void set_shading_model_impl(
        render_id shading_model_id,
        const std::function<std::unique_ptr<shading_model>()>& creator);

    using material_header = header;

    material_header& get_header()
    {
        material::mark_dirty(material::DIRTY_FLAG_CONSTANT);
        return *reinterpret_cast<material_header*>(m_constant.data());
    }

    const material_header& get_header() const
    {
        return *reinterpret_cast<const material_header*>(m_constant.data());
    }

    template <typename T>
    T& get_constant()
    {
        assert(m_constant.size() == sizeof(material_header) + sizeof(T));
        material::mark_dirty(material::DIRTY_FLAG_CONSTANT);
        return *reinterpret_cast<T*>(m_constant.data() + sizeof(material_header));
    }

    template <typename T>
    const T& get_constant() const
    {
        assert(m_constant.size() == sizeof(material_header) + sizeof(T));
        return *reinterpret_cast<const T*>(m_constant.data() + sizeof(material_header));
    }

    void mark_dirty(dirty_flags dirty_flags);

private:
    surface_type m_surface_type;

    render_id m_material_id{INVALID_RENDER_ID};

    material_path m_material_path{MATERIAL_PATH_AUTO};

    rdg_raster_pipeline m_raster_pipeline{};
    render_id m_raster_pipeline_id{0};

    rdg_compute_pipeline m_resolve_pipeline{};

    shadow_cull_mode m_shadow_cull_mode{SHADOW_CULL_MODE_AUTO};

    std::string m_name;
    std::string m_shader_path;

    std::vector<std::uint8_t> m_constant;

    dirty_flags m_dirty_flags{0};
};

using material_header = material::header;

template <typename Constant>
class material_instance : public material
{
public:
    using constant_type = Constant;

    material_instance(std::string_view name)
        : material(name, sizeof(Constant))
    {
    }

protected:
    constant_type& get_constant() noexcept
    {
        material::mark_dirty(material::DIRTY_FLAG_CONSTANT);
        return material::get_constant<constant_type>();
    }

    const constant_type& get_constant() const noexcept
    {
        return material::get_constant<constant_type>();
    }
};
} // namespace violet
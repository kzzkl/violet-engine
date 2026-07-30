#pragma once

#include "components/camera_component.hpp"
#include "graphics/atmosphere.hpp"
#include "graphics/material_manager.hpp"
#include "graphics/render_scene/render_scene_module.hpp"
#include <vector>

namespace violet
{
class gpu_buffer_uploader;
class vsm_manager;

class render_scene
{
public:
    render_scene(vsm_manager* vsm_manager);
    render_scene(const render_scene&) = delete;

    ~render_scene();

    render_scene& operator=(const render_scene&) = delete;

    template <typename T>
    T& get_module() noexcept
    {
        if constexpr (std::is_same_v<T, render_scene_camera>)
        {
            return *m_camera_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_mesh>)
        {
            return *m_mesh_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_light>)
        {
            return *m_light_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_shadow>)
        {
            return *m_shadow_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_environment>)
        {
            return *m_environment_module;
        }
        else
        {
            static_assert(sizeof(T) == 0, "Unknown module type for render_scene::get_module");
        }
    }

    template <typename T>
    const T& get_module() const noexcept
    {
        if constexpr (std::is_same_v<T, render_scene_camera>)
        {
            return *m_camera_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_mesh>)
        {
            return *m_mesh_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_light>)
        {
            return *m_light_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_shadow>)
        {
            return *m_shadow_module;
        }
        else if constexpr (std::is_same_v<T, render_scene_environment>)
        {
            return *m_environment_module;
        }
        else
        {
            static_assert(sizeof(T) == 0, "Unknown module type for render_scene::get_module");
        }
    }

    void end_frame(gpu_buffer_uploader* uploader);
    void reset_states();

private:
    std::unique_ptr<render_scene_camera> m_camera_module;
    std::unique_ptr<render_scene_mesh> m_mesh_module;
    std::unique_ptr<render_scene_light> m_light_module;
    std::unique_ptr<render_scene_shadow> m_shadow_module;
    std::unique_ptr<render_scene_environment> m_environment_module;

    render_scene_context m_context;
    rhi_ptr<rhi_parameter> m_scene_parameter;

    friend class render_context;
};

class camera_component;
class camera_component_meta;
class render_context
{
public:
    struct camera_info
    {
        render_id id;
        camera_type type;
        float near;
        float far;
        float perspective_fov;
        float orthographic_size;
        vec3f position;
        mat4f matrix_v;
        mat4f matrix_p;
        mat4f matrix_vp;
        mat4f matrix_vp_no_jitter;
        background_type background;

        rhi_parameter* parameter;
    };

    struct scene_info
    {
        std::uint32_t instance_count;
        std::uint32_t draw_call_capacity;
        std::uint32_t draw_call_count;
        std::uint32_t batch_capacity;

        std::uint32_t light_count;

        rhi_buffer* shadow_light_buffer;
        std::uint32_t shadow_light_count;

        rhi_buffer* vsm_buffer;
        std::uint32_t vsm_count;
        rhi_buffer* vsm_clipmap_buffer;
        rhi_texture* vsm_hzb;
        rhi_buffer* vsm_virtual_page_table;
        rhi_buffer* vsm_physical_page_table;
        rhi_texture* vsm_physical_shadow_map_static;
        rhi_texture* vsm_physical_shadow_map_final;

        rhi_texture* environment_map;
        rhi_texture* prefilter_map;
        rhi_buffer* irradiance_sh;

        atmosphere atmosphere;
        vec3f sun_direction;
        vec3f sun_irradiance;
        rhi_texture* transmittance_lut;
        rhi_texture* multi_scattering_lut;

        bool atmosphere_diry;

        rhi_parameter* parameter;
    };

    render_context(const camera_component* camera, const camera_component_meta* camera_meta);

    const camera_info& get_camera() const noexcept
    {
        return m_camera_info;
    }

    const scene_info& get_scene() const noexcept
    {
        return m_scene_info;
    }

    rhi_texture* get_render_target() const noexcept
    {
        return m_render_target;
    }

    rhi_viewport get_viewport() const noexcept
    {
        return m_viewport;
    }

    std::vector<rhi_scissor_rect> get_scissor_rects() const noexcept
    {
        return m_scissor_rects;
    }

    template <typename T>
    const T& get_module() const noexcept
    {
        return m_scene->get_module<T>();
    }

private:
    camera_info m_camera_info{};
    scene_info m_scene_info{};

    rhi_texture* m_render_target;
    rhi_viewport m_viewport;
    std::vector<rhi_scissor_rect> m_scissor_rects;

    render_scene* m_scene{nullptr};
};
} // namespace violet
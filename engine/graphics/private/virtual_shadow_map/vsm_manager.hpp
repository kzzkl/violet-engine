#pragma once

#include "components/light_component.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
struct vsm_directional_light_data
{
    vec3f light_position;
    vec3f light_direction;
    vec3f camera_position;
};

class gpu_buffer_uploader;
class vsm_manager
{
public:
    vsm_manager();
    vsm_manager(const vsm_manager&) = delete;

    vsm_manager& operator=(const vsm_manager&) = delete;

    render_id add_vsm(light_type light_type);
    void remove_vsm(render_id vsm_id);

    void set_vsm(render_id vsm_id, const vsm_directional_light_data& light);

    void update(gpu_buffer_uploader* uploader);

    rhi_buffer* get_vsm_buffer()
    {
        return m_vsms.get_buffer()->get_rhi();
    }

    rhi_buffer* get_vsm_virtual_page_table()
    {
        return m_virtual_page_table.get();
    }

    rhi_buffer* get_vsm_physical_page_table()
    {
        return m_physical_page_table.get();
    }

    rhi_texture* get_vsm_physical_texture()
    {
        return m_physical_texture.get();
    }

    rhi_texture* get_vsm_hzb()
    {
        return m_hzb.get();
    }

private:
    struct gpu_vsm
    {
        struct gpu_type
        {
            vec2i page_coord;
            std::uint32_t cache_epoch;
            float view_z_radius;
            mat4f matrix_v;
            mat4f matrix_p;
            mat4f matrix_vp;
        };

        light_type light_type;

        vec2i page_coord;
        mat4f matrix_v;
        mat4f matrix_p;

        std::uint32_t cache_epoch;

        float view_z;
        float view_z_radius;
    };

    static constexpr std::uint32_t get_vsm_count(light_type light_type);

    gpu_block_sparse_array<gpu_vsm> m_vsms;

    rhi_ptr<rhi_buffer> m_virtual_page_table;
    rhi_ptr<rhi_buffer> m_physical_page_table;

    rhi_ptr<rhi_texture> m_physical_texture;
    rhi_ptr<rhi_texture> m_hzb;

    std::unordered_map<render_id, vsm_directional_light_data> m_directional_lights;
};
} // namespace violet
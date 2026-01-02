#pragma once

#include "components/light_component.hpp"
#include "graphics/gpu_array.hpp"
#include "graphics/render_device.hpp"

namespace violet
{
class gpu_buffer_uploader;
class vsm_manager
{
public:
    vsm_manager();
    vsm_manager(const vsm_manager&) = delete;

    vsm_manager& operator=(const vsm_manager&) = delete;

    render_id add_vsm(
        light_type light_type,
        render_id light_id,
        render_id camera_id = INVALID_RENDER_ID);
    void remove_vsm(render_id vsm_id);

    void set_vsm_light(render_id vsm_id, const vec3f& light_position, const vec3f& light_direction);
    void set_vsm_camera(render_id vsm_id, const vec3f& camera_position);

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

private:
    struct gpu_vsm
    {
        struct gpu_type
        {
            vec2i page_coord;
            vec2i page_offset;
            mat4f matrix_vp;
        };

        light_type light_type;

        render_id light_id;
        render_id camera_id;

        vec3f light_position;
        vec3f light_direction;
        vec3f camera_position;

        vec2i page_coord;
        vec2i page_offset;
        mat4f matrix_vp;

        bool dirty;
    };

    static constexpr std::uint32_t get_vsm_count(light_type light_type);

    gpu_block_sparse_array<gpu_vsm> m_vsms;
    std::vector<render_id> m_dirty_vsms;

    rhi_ptr<rhi_buffer> m_virtual_page_table;
    rhi_ptr<rhi_buffer> m_physical_page_table;
};
} // namespace violet
#pragma once

#include "graphics/gpu_array.hpp"
#include "graphics/render_scene/render_scene_module.hpp"
#include "math/sphere.hpp"
#include <array>

namespace violet
{
class vsm_manager;
class render_scene_shadow : public render_scene_module
{
public:
    render_scene_shadow(vsm_manager* vsm_manager);

    void add_shadow(render_id light_id);
    void remove_shadow(render_id light_id);

    void update(render_scene_context& context, gpu_buffer_uploader& uploader) override;

    rhi_buffer* get_shadow_light_buffer() const noexcept
    {
        return m_shadow_addresses.get_buffer()->get_rhi();
    }

    std::uint32_t get_shadow_light_count() const noexcept
    {
        return m_shadow_addresses.get_size();
    }

    rhi_buffer* get_vsm_clipmap_buffer() const noexcept
    {
        return m_clipmaps.get_buffer()->get_rhi();
    }

    rhi_buffer* get_vsm_buffer() const noexcept;

    std::uint32_t get_vsm_count() const noexcept;

    rhi_texture* get_vsm_hzb() const noexcept;

    rhi_buffer* get_vsm_virtual_page_table() const noexcept;

    rhi_buffer* get_vsm_physical_page_table() const noexcept;

    rhi_texture* get_vsm_physical_shadow_map_static() const noexcept;

    rhi_texture* get_vsm_physical_shadow_map_final() const noexcept;

    rhi_buffer* get_vsm_invalidation_buffer() const noexcept
    {
        return m_invalidations.get_buffer()->get_rhi();
    }

    std::uint32_t get_vsm_invalidation_count() const noexcept
    {
        return m_invalidations.get_size();
    }

private:
    struct shadow_address
    {
        using gpu_type = vec2u;

        render_id light_id;

        // when the light type is directional light, vsm_address points to the address of
        // clipmap_buffer. for other types, vsm_address is the id of vsm.
        render_id vsm_address{INVALID_RENDER_ID};
    };

    struct clipmap
    {
        using gpu_type = std::array<std::uint32_t, 16>;

        struct vsm
        {
            render_id vsm_id;
            render_id light_id;
            render_id camera_id;
        };

        std::array<vsm, 16> vsms;
    };

    struct light_snapshot
    {
        render_id light_id;
        render_id shadow_address;

        vec3f position;
        vec3f direction;
    };

    struct camera_snapshot
    {
        render_id camera_id;
        vec3f position;
    };

    void deallocate_vsm(render_scene_context& context);
    void allocate_vsm(render_scene_context& context);
    void update_vsm(render_scene_context& context);

    void update_shadow_addresss(gpu_buffer_uploader& uploader);
    void update_clipmaps(gpu_buffer_uploader& uploader);
    void update_invalidation(render_scene_context& context, gpu_buffer_uploader& uploader);

    std::unordered_map<render_id, light_snapshot> m_lights;
    std::unordered_map<render_id, camera_snapshot> m_cameras;

    gpu_dense_array<shadow_address> m_shadow_addresses;
    gpu_sparse_array<clipmap> m_clipmaps;

    std::vector<render_id> m_added_lights;
    std::vector<render_id> m_removed_lights;

    struct page_invalidation
    {
        using gpu_type = vec4f;
        sphere3f sphere;
    };
    gpu_sparse_array<page_invalidation> m_invalidations;

    vsm_manager* m_vsm_manager;
};
} // namespace violet
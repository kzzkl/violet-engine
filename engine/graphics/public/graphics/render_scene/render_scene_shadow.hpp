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

    rhi_buffer* get_clipmap_buffer() const noexcept
    {
        return m_clipmaps.get_buffer()->get_rhi();
    }

    rhi_buffer* get_vsm_buffer() const noexcept;

    std::uint32_t get_vsm_count() const noexcept;

    rhi_texture* get_hzb() const noexcept;

    rhi_buffer* get_virtual_page_table() const noexcept;

    rhi_buffer* get_physical_page_table() const noexcept;

    rhi_texture* get_physical_shadow_map_static() const noexcept;

    rhi_texture* get_physical_shadow_map_final() const noexcept;

    rhi_buffer* get_invalidation_regions_buffer() const noexcept
    {
        return m_invalidation_regions.get_buffer()->get_rhi();
    }

    std::uint32_t get_invalidation_region_count() const noexcept
    {
        return m_invalidation_regions.get_size();
    }

    render_id get_vsm_id(render_id light_id, render_id camera_id = INVALID_RENDER_ID) const;

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

        bool is_directional_light;
    };

    void deallocate_vsm(render_scene_context& context);
    void allocate_vsm(render_scene_context& context);
    void update_vsm(render_scene_context& context);

    void update_shadow_addresss(gpu_buffer_uploader& uploader);
    void update_clipmaps(gpu_buffer_uploader& uploader);
    void update_invalidation(render_scene_context& context, gpu_buffer_uploader& uploader);

    std::unordered_map<render_id, light_snapshot> m_lights;

    gpu_dense_array<shadow_address> m_shadow_addresses;
    gpu_sparse_array<clipmap> m_clipmaps;

    std::vector<render_id> m_added_lights;
    std::vector<render_id> m_removed_lights;

    struct invalidation_region
    {
        using gpu_type = vec4f;
        sphere3f sphere;
    };
    gpu_append_array<invalidation_region> m_invalidation_regions;

    vsm_manager* m_vsm_manager;
};
} // namespace violet
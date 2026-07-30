#pragma once

#include "common/allocator.hpp"
#include "components/camera_component.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_scene/render_scene_module.hpp"

namespace violet
{
class render_scene_camera : public render_scene_module
{
public:
    struct camera_data
    {
        vec3f position;

        background_type background_type{BACKGROUND_TYPE_SKYBOX};
        rhi_ptr<rhi_texture> environment_map;
        rhi_ptr<rhi_buffer> irradiance_sh;
        rhi_ptr<rhi_texture> prefilter_map;

        bool dirty;
        bool valid;
    };

    render_id add_camera();
    void remove_camera(render_id camera_id);
    void set_camera_position(render_id camera_id, const vec3f& position);
    void set_camera_background(render_id camera_id, background_type background_type);

    const camera_data& get_camera(render_id camera_id) const
    {
        return m_cameras[camera_id];
    }

    void update(render_scene_context& context, gpu_buffer_uploader& uploader) override;
    void reset() override;

    template <typename Functor>
    void each_camera(Functor&& functor)
    {
        for (render_id camera_id = 0; camera_id < m_cameras.size(); ++camera_id)
        {
            if (m_cameras[camera_id].valid)
            {
                functor(camera_id);
            }
        }
    }

    template <typename Functor>
    void each_added_camera(Functor&& functor)
    {
        for (auto camera_id : m_added_cameras)
        {
            functor(camera_id);
        }
    }

    template <typename Functor>
    void each_removed_camera(Functor&& functor)
    {
        for (auto camera_id : m_removed_cameras)
        {
            functor(camera_id);
        }
    }

private:
    std::vector<camera_data> m_cameras;

    index_allocator m_camera_allocator;

    std::vector<render_id> m_added_cameras;
    std::vector<render_id> m_removed_cameras;
};
} // namespace violet
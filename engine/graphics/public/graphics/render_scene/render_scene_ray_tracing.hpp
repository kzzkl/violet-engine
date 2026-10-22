#pragma once

#include "common/allocator.hpp"
#include "graphics/geometry.hpp"
#include "graphics/render_device.hpp"
#include "graphics/render_scene/render_scene_module.hpp"

namespace violet
{
class render_scene_ray_tracing : public render_scene_module
{
public:
    render_scene_ray_tracing();

    render_id add_instance();
    void set_instance_geometry(render_id instance_id, geometry* geometry);
    void set_instance_matrix(render_id instance_id, const mat4f& matrix);
    void remove_instance(render_id instance_id);

    void upload(render_scene_context& context, gpu_buffer_uploader& uploader) override;
    void update(render_scene_context& context, render_graph& graph) override;

private:
    struct instance_data
    {
        geometry* geometry;
        float transform[12];

        bool valid;
    };

    std::vector<instance_data> m_instances;
    index_allocator m_instance_allocator;

    rhi_ptr<rhi_acceleration_structure> m_tlas;
};
} // namespace violet
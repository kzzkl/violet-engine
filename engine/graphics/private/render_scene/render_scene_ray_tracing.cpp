#include "graphics/render_scene/render_scene_ray_tracing.hpp"
#include "graphics/render_graph/render_graph.hpp"

namespace violet
{
render_scene_ray_tracing::render_scene_ray_tracing() {}

render_id render_scene_ray_tracing::add_instance()
{
    render_id id = m_instance_allocator.allocate();
    if (id >= m_instances.size())
    {
        m_instances.resize(id + 1);
    }

    m_instances[id] = {
        .valid = true,
    };

    return id;
} // namespace violet

void render_scene_ray_tracing::set_instance_geometry(render_id instance_id, geometry* geometry)
{
    m_instances[instance_id].geometry = geometry;
}

void render_scene_ray_tracing::set_instance_matrix(render_id instance_id, const mat4f& matrix)
{
    float* transform = m_instances[instance_id].transform;

    transform[0] = matrix[0][0];
    transform[1] = matrix[1][0];
    transform[2] = matrix[2][0];
    transform[3] = matrix[3][0];

    transform[4] = matrix[0][1];
    transform[5] = matrix[1][1];
    transform[6] = matrix[2][1];
    transform[7] = matrix[3][1];

    transform[8] = matrix[0][2];
    transform[9] = matrix[1][2];
    transform[10] = matrix[2][2];
    transform[11] = matrix[3][2];
}

void render_scene_ray_tracing::remove_instance(render_id instance_id)
{
    m_instance_allocator.free(instance_id);
    m_instances[instance_id] = {
        .valid = false,
    };
}

void render_scene_ray_tracing::upload(render_scene_context& context, gpu_buffer_uploader& uploader)
{
}

void render_scene_ray_tracing::update(render_scene_context& context, render_graph& graph)
{
    if (m_tlas == nullptr && !m_instances.empty())
    {
        auto& device = render_device::instance();

        std::vector<rhi_acceleration_structure_desc::instance> instances;
        for (auto& instance : m_instances)
        {
            if (!instance.valid)
            {
                continue;
            }

            rhi_acceleration_structure_desc::instance instance_desc = {
                .blas = instance.geometry->get_blas()->get_blas(),
            };
            std::memcpy(instance_desc.transform, instance.transform, sizeof(float) * 12);

            instances.push_back(instance_desc);
        }

        m_tlas = device.create_acceleration_structure({
            .type = RHI_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
            .flags = RHI_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE,
            .geometry_type = RHI_GEOMETRY_TYPE_INSTANCES,
            .instances =
                {
                    .instances = instances.data(),
                    .instance_count = static_cast<std::uint32_t>(instances.size()),
                },
        });
    }
}
} // namespace violet
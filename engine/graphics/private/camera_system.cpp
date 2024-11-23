#include "graphics/camera_system.hpp"
#include "components/camera.hpp"
#include "components/camera_render_data.hpp"
#include "components/transform.hpp"

namespace violet
{
camera_system::camera_system()
    : engine_system("camera")
{
}

bool camera_system::initialize(const dictionary& config)
{
    task_graph& task_graph = get_task_graph();
    task_group& post_update = task_graph.get_group("Post Update Group");

    task& update_transform = task_graph.get_task("Update Transform");

    task_graph.add_task()
        .set_name("Update Camera")
        .set_group(post_update)
        .add_dependency(update_transform)
        .set_execute(
            [this]()
            {
                update_render_data();
                m_system_version = get_world().get_version();
            });

    auto& world = get_world();
    world.register_component<camera>();
    world.register_component<camera_render_data>();

    return true;
}

void camera_system::update_render_data()
{
    auto& world = get_world();

    world.get_view().read<camera>().read<transform_world>().write<camera_render_data>().each(
        [](const camera& camera, const transform_world& transform, camera_render_data& render_data)
        {
            if (camera.render_targets.empty())
            {
                return;
            }

            shader::camera_data data = {};

            rhi_texture_extent extent = camera.get_extent();
            float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

            mat4f_simd projection =
                matrix::perspective_reverse_z<simd>(camera.fov, aspect, camera.near, camera.far);
            mat4f_simd view = matrix::inverse(math::load(transform.matrix));
            mat4f_simd view_projection = matrix::mul(view, projection);

            math::store(projection, data.projection);
            math::store(view, data.view);
            math::store(view_projection, data.view_projection);

            data.position = transform.get_position();

            if (render_data.parameter == nullptr)
            {
                render_data.parameter = render_device::instance().create_parameter(shader::camera);
            }

            render_data.parameter->set_uniform(0, &data, sizeof(shader::camera_data), 0);
        },
        [this](auto& view)
        {
            return view.is_updated<camera>(m_system_version) ||
                   view.is_updated<transform_world>(m_system_version);
        });
}
} // namespace violet
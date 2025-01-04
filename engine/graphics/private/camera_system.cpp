#include "camera_system.hpp"
#include "components/camera_component.hpp"
#include "components/camera_meta_component.hpp"
#include "components/transform_component.hpp"

namespace violet
{
camera_system::camera_system()
    : system("camera")
{
}

bool camera_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<camera_component>();
    world.register_component<camera_meta_component>();

    return true;
}

void camera_system::update()
{
    auto& world = get_world();

    world.get_view()
        .read<camera_component>()
        .read<transform_world_component>()
        .write<camera_meta_component>()
        .each(
            [](const camera_component& camera,
               const transform_world_component& transform,
               camera_meta_component& camera_meta)
            {
                if (camera.render_targets.empty())
                {
                    return;
                }

                shader::camera_data data = {
                    .position = transform.get_position(),
                    .fov = camera.fov,
                };

                rhi_texture_extent extent = camera.get_extent();
                float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

                mat4f_simd projection = matrix::perspective_reverse_z<simd>(
                    camera.fov,
                    aspect,
                    camera.near,
                    camera.far);
                mat4f_simd view = matrix::inverse(math::load(transform.matrix));
                mat4f_simd view_projection = matrix::mul(view, projection);
                mat4f_simd view_projection_inv = matrix::inverse(view_projection);

                math::store(projection, data.projection);
                math::store(view, data.view);
                math::store(view_projection, data.view_projection);
                math::store(view_projection_inv, data.view_projection_inv);

                if (camera_meta.parameter == nullptr)
                {
                    camera_meta.parameter =
                        render_device::instance().create_parameter(shader::camera);
                }

                camera_meta.parameter->set_constant(0, &data, sizeof(shader::camera_data));
            },
            [this](auto& view)
            {
                return view.template is_updated<camera_component>(m_system_version) ||
                       view.template is_updated<transform_world_component>(m_system_version);
            });
}
} // namespace violet
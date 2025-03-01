#include "camera_system.hpp"
#include "components/camera_component.hpp"
#include "components/camera_meta_component.hpp"
#include "components/transform_component.hpp"
#include "graphics/passes/taa_pass.hpp"

namespace violet
{
namespace
{
shader::camera_data get_camera_data(
    const camera_component& camera,
    const transform_world_component& transform)
{
    static const std::array<vec2f, 8> halton_sequence = {
        vec2f{0.500000f, 0.333333f},
        vec2f{0.250000f, 0.666667f},
        vec2f{0.750000f, 0.111111f},
        vec2f{0.125000f, 0.444444f},
        vec2f{0.625000f, 0.777778f},
        vec2f{0.375000f, 0.222222f},
        vec2f{0.875000f, 0.555556f},
        vec2f{0.062500f, 0.888889f},
    };

    shader::camera_data result = {
        .position = transform.get_position(),
        .fov = camera.fov,
    };

    rhi_texture_extent extent = camera.get_extent();
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    mat4f_simd projection =
        matrix::perspective_reverse_z<simd>(camera.fov, aspect, camera.near, camera.far);
    mat4f_simd view = matrix::inverse(math::load(transform.matrix));

    mat4f_simd view_projection = matrix::mul(view, projection);
    math::store(view_projection, result.view_projection_no_jitter);

    auto* taa = camera.get_feature<taa_render_feature>();
    if (taa && taa->is_enable())
    {
        std::size_t index = render_device::instance().get_frame_count() % halton_sequence.size();

        vec2f jitter = halton_sequence[index] - vec2f{0.5f, 0.5f};
        jitter.x = jitter.x * 2.0f / static_cast<float>(extent.width);
        jitter.y = jitter.y * 2.0f / static_cast<float>(extent.height);

        vec4f_simd offset = math::load(jitter);
        projection[2] = vector::add(projection[2], offset);

        view_projection = matrix::mul(view, projection);

        result.jitter = jitter;
    }

    math::store(view, result.view);
    math::store(projection, result.projection);
    math::store(matrix::inverse(projection), result.projection_inv);
    math::store(view_projection, result.view_projection);
    math::store(matrix::inverse(view_projection), result.view_projection_inv);

    return result;
}
} // namespace

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
            [this](
                const camera_component& camera,
                const transform_world_component& transform,
                camera_meta_component& camera_meta)
            {
                if (!camera.has_render_target())
                {
                    return;
                }

                shader::camera_data data = get_camera_data(camera, transform);
                data.prev_view_projection = camera_meta.view_projection;
                data.prev_view_projection_no_jitter = camera_meta.view_projection_no_jitter;
                camera_meta.view_projection = data.view_projection;
                camera_meta.view_projection_no_jitter = data.view_projection_no_jitter;

                if (camera_meta.parameter == nullptr)
                {
                    camera_meta.parameter =
                        render_device::instance().create_parameter(shader::camera);
                }

                camera_meta.parameter->set_uniform(0, &data, sizeof(shader::camera_data));
            });

    m_system_version = world.get_version();
}
} // namespace violet
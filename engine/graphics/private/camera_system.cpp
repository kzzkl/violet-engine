#include "camera_system.hpp"
#include "components/camera_component.hpp"
#include "components/camera_component_meta.hpp"
#include "components/transform_component.hpp"
#include "graphics/renderers/features/taa_render_feature.hpp"

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

    rhi_texture_extent extent = camera.get_extent();

    shader::camera_data result = {
        .position = transform.get_position(),
        .fov = camera.fov,
        .near = camera.near,
        .far = camera.far,
        .width = static_cast<float>(extent.width),
        .height = static_cast<float>(extent.height),
    };

    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    mat4f_simd matrix_p =
        camera.far == std::numeric_limits<float>::infinity() ?
            matrix::perspective_reverse_z<simd>(camera.fov, aspect, camera.near) :
            matrix::perspective_reverse_z<simd>(camera.fov, aspect, camera.near, camera.far);
    mat4f_simd matrix_v = matrix::inverse(math::load(transform.matrix));

    mat4f_simd matrix_vp = matrix::mul(matrix_v, matrix_p);
    math::store(matrix_vp, result.matrix_vp_no_jitter);

    auto* taa = camera.renderer->get_feature<taa_render_feature>();
    if (taa && taa->is_enable())
    {
        std::size_t index = render_device::instance().get_frame_count() % halton_sequence.size();

        vec2f jitter = halton_sequence[index] - vec2f{0.5f, 0.5f};
        jitter.x = jitter.x * 2.0f / static_cast<float>(extent.width);
        jitter.y = jitter.y * 2.0f / static_cast<float>(extent.height);

        vec4f_simd offset = math::load(jitter);
        matrix_p[2] = vector::add(matrix_p[2], offset);

        matrix_vp = matrix::mul(matrix_v, matrix_p);

        result.jitter = jitter;
    }

    math::store(matrix_v, result.matrix_v);
    math::store(matrix_p, result.matrix_p);
    math::store(matrix::inverse(matrix_p), result.matrix_p_inv);
    math::store(matrix_vp, result.matrix_vp);
    math::store(matrix::inverse(matrix_vp), result.matrix_vp_inv);

    return result;
}

rhi_texture_extent get_previous_pow2_extent(const rhi_texture_extent& extent)
{
    rhi_texture_extent result = {.width = 1, .height = 1};
    while ((result.width << 1) < extent.width)
    {
        result.width <<= 1;
    }

    while ((result.height << 1) < extent.height)
    {
        result.height <<= 1;
    }

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
    world.register_component<camera_component_meta>();

    return true;
}

void camera_system::update()
{
    auto& world = get_world();

    world.get_view()
        .read<camera_component>()
        .read<transform_world_component>()
        .write<camera_component_meta>()
        .each(
            [](const camera_component& camera,
               const transform_world_component& transform,
               camera_component_meta& camera_meta)
            {
                if (!camera.has_render_target() || camera.renderer == nullptr)
                {
                    return;
                }

                if (camera_meta.parameter == nullptr)
                {
                    camera_meta.parameter =
                        render_device::instance().create_parameter(shader::camera);
                }

                if (camera_meta.hzb == nullptr || camera_meta.extent != camera.get_extent())
                {
                    camera_meta.extent = camera.get_extent();

                    rhi_texture_extent extent = get_previous_pow2_extent(camera_meta.extent);

                    std::uint32_t max_size = std::max(extent.width, extent.height);
                    std::uint32_t level_count =
                        static_cast<std::uint32_t>(std::floor(std::log2(max_size))) + 1;

                    camera_meta.hzb = render_device::instance().create_texture({
                        .extent = extent,
                        .format = RHI_FORMAT_R32_FLOAT,
                        .flags = RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
                        .level_count = level_count,
                        .layer_count = 1,
                        .samples = RHI_SAMPLE_COUNT_1,
                        .layout = RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
                    });
                }

                shader::camera_data data = get_camera_data(camera, transform);
                data.prev_matrix_v = camera_meta.matrix_v;
                data.prev_matrix_p = camera_meta.matrix_p;
                data.prev_matrix_vp = camera_meta.matrix_vp;
                data.prev_matrix_vp_no_jitter = camera_meta.matrix_vp_no_jitter;
                camera_meta.parameter->set_uniform(0, &data, sizeof(shader::camera_data));

                camera_meta.matrix_v = data.matrix_v;
                camera_meta.matrix_p = data.matrix_p;
                camera_meta.matrix_vp = data.matrix_vp;
                camera_meta.matrix_vp_no_jitter = data.matrix_vp_no_jitter;
            });

    m_system_version = world.get_version();
}
} // namespace violet
#include "graphics/renderers/passes/skybox_pass.hpp"
#include "graphics/render_interface.hpp"

namespace violet
{
struct sky_constant_data
{
    vec3f sun_direction;
    float sun_angular_radius;
    vec3f sun_irradiance;
    float planet_radius;
    float atmosphere_radius;
    std::uint32_t skybox_texture;
    std::uint32_t sky_view_lut;
    std::uint32_t transmittance_lut;
};

struct sky_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky.hlsl";

    using constant_data = sky_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 2, .desc = camera},
    };
};

struct sky_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/atmosphere/sky.hlsl";

    using constant_data = sky_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

void skybox_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Skybox Pass");

    struct pass_data
    {
        rhi_texture_srv* environment_map;
    };

    graph.add_pass<pass_data>(
        "Skybox Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            rhi_attachment_load_op load_op =
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;

            pass.add_render_target(parameter.render_target, load_op);
            pass.set_depth_stencil(parameter.depth_buffer, load_op);

            const auto& context = graph.get_context();
            data.environment_map =
                context.get_environment_map()->get_srv(RHI_TEXTURE_DIMENSION_CUBE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            auto& device = render_device::instance();

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<sky_vs>(),
                .fragment_shader = device.get_shader<sky_fs>(),
                .depth_stencil_state =
                    device.get_depth_stencil_state<true, false, RHI_COMPARE_OP_EQUAL>(),
            };

            command.set_pipeline(pipeline);

            command.set_constant(
                sky_constant_data{
                    .skybox_texture = data.environment_map->get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.draw(0, 36);
        });
}
} // namespace violet
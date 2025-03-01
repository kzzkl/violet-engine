#include "graphics/passes/lighting/physical_pass.hpp"
#include "graphics/resources/brdf_lut.hpp"

namespace violet
{
struct physical_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/lighting/physical.hlsl";

    struct constant_data
    {
        std::uint32_t albedo;
        std::uint32_t material;
        std::uint32_t normal;
        std::uint32_t depth;
        std::uint32_t emissive;
        std::uint32_t ao_buffer;
        std::uint32_t brdf_lut;
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, camera},
    };
};

void physical_pass::add(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_srv gbuffer_albedo;
        rdg_texture_srv gbuffer_material;
        rdg_texture_srv gbuffer_normal;
        rdg_texture_srv gbuffer_depth;
        rdg_texture_srv gbuffer_emissive;
        rdg_texture_srv ao_buffer;
    };

    graph.add_pass<pass_data>(
        "Physical Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.add_render_target(
                parameter.render_target,
                parameter.clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD);
            pass.set_depth_stencil(parameter.depth_buffer, RHI_ATTACHMENT_LOAD_OP_LOAD);

            data.gbuffer_albedo =
                pass.add_texture_srv(parameter.gbuffer_albedo, RHI_PIPELINE_STAGE_FRAGMENT);
            data.gbuffer_material =
                pass.add_texture_srv(parameter.gbuffer_material, RHI_PIPELINE_STAGE_FRAGMENT);
            data.gbuffer_normal =
                pass.add_texture_srv(parameter.gbuffer_normal, RHI_PIPELINE_STAGE_FRAGMENT);
            data.gbuffer_depth =
                pass.add_texture_srv(parameter.gbuffer_depth, RHI_PIPELINE_STAGE_FRAGMENT);
            data.gbuffer_emissive =
                pass.add_texture_srv(parameter.gbuffer_emissive, RHI_PIPELINE_STAGE_FRAGMENT);

            if (parameter.ao_buffer != nullptr)
            {
                data.ao_buffer =
                    pass.add_texture_srv(parameter.ao_buffer, RHI_PIPELINE_STAGE_FRAGMENT);
            }
            else
            {
                data.ao_buffer = rdg_texture_srv();
            }
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            physical_fs::constant_data constant = {
                .albedo = data.gbuffer_albedo.get_bindless(),
                .material = data.gbuffer_material.get_bindless(),
                .normal = data.gbuffer_normal.get_bindless(),
                .depth = data.gbuffer_depth.get_bindless(),
                .emissive = data.gbuffer_emissive.get_bindless(),
                .brdf_lut = device.get_buildin_texture<brdf_lut>()->get_srv()->get_bindless(),
            };

            std::vector<std::wstring> defines;
            if (data.ao_buffer)
            {
                defines.emplace_back(L"-DUSE_AO_BUFFER");
                constant.ao_buffer = data.ao_buffer.get_bindless();
            }

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<fullscreen_vs>(),
                .fragment_shader = device.get_shader<physical_fs>(defines),
            };
            pipeline.depth_stencil.stencil_enable = true;
            pipeline.depth_stencil.stencil_front = {
                .compare_op = RHI_COMPARE_OP_EQUAL,
                .reference = SHADING_MODEL_PHYSICAL,
            };
            pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

            command.set_pipeline(pipeline);
            command.set_constant(constant);
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);
            command.draw_fullscreen();
        });
}
} // namespace violet
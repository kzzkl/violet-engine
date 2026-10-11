#include "sdf_debug_pass.hpp"
#include "math/matrix.hpp"
#include <string>
#include <vector>

namespace violet
{
struct sdf_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/sdf.hlsl";

    struct constant_data
    {
        vec3f bounding_box_min;
        std::uint32_t render_target;
        vec3f bounding_box_max;
        std::uint32_t depth_buffer;

        // The shader marches in the local space of the bounding box, so the
        // inverse of matrix_m is what it needs.
        mat4f matrix_m_inv;

        std::uint32_t sdf;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

struct sdf_bounds_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/sdf_bounds.hlsl";

    struct constant_data
    {
        vec3f bounding_box_min;
        std::uint32_t padding0;
        vec3f bounding_box_max;
        std::uint32_t padding1;

        vec3u brick_count;
        std::uint32_t padding2;

        mat4f matrix_m;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

struct sdf_bounds_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/sdf_bounds.hlsl";

    using constant_data = sdf_bounds_vs::constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void sdf_debug_pass::add(render_graph& graph, const parameter& parameter)
{
    add_sdf_pass(graph, parameter);

    if (parameter.render_bounds)
    {
        add_bounds_pass(graph, parameter);
    }
}

void sdf_debug_pass::add_sdf_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_uav render_target;
        rdg_texture_uav depth_buffer;

        rhi_texture_srv* sdf;

        box3f bounding_box;
        mat4f matrix_m_inv;
    };

    graph.add_pass<pass_data>(
        "SDF",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target =
                pass.add_texture_uav(parameter.render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.depth_buffer =
                pass.add_texture_uav(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            data.sdf = parameter.sdf->get_srv(RHI_TEXTURE_DIMENSION_3D);

            data.bounding_box = parameter.bounding_box;
            data.matrix_m_inv = matrix::inverse(parameter.matrix_m);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_cs>(),
            });

            sdf_cs::constant_data constant = {
                .bounding_box_min = data.bounding_box.min,
                .render_target = data.render_target.get_bindless(),
                .bounding_box_max = data.bounding_box.max,
                .depth_buffer = data.depth_buffer.get_bindless(),
                .matrix_m_inv = data.matrix_m_inv,
                .sdf = data.sdf->get_bindless(),
            };
            command.set_constant(constant);

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            auto extent = data.render_target.get_extent();
            command.dispatch_2d(extent.width, extent.height);
        });
}

void sdf_debug_pass::add_bounds_pass(render_graph& graph, const parameter& parameter)
{
    struct pass_data
    {
        rdg_texture_rtv render_target;
        rdg_texture_dsv depth_buffer;

        vec3u brick_count;

        box3f bounding_box;
        mat4f matrix_m;

        std::uint32_t vertex_count;
    };

    graph.add_pass<pass_data>(
        "SDF Bounds",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.add_render_target(parameter.render_target, RHI_ATTACHMENT_LOAD_OP_LOAD);
            pass.set_depth_stencil(parameter.depth_buffer, RHI_ATTACHMENT_LOAD_OP_LOAD);

            const rhi_extent& sdf_extent = parameter.sdf->get_extent();

            // A brick covers 8x8x8 voxels of the SDF texture.
            auto get_brick_count = [](std::uint32_t voxel_count)
            {
                constexpr std::uint32_t brick_size = 8;
                return (voxel_count + brick_size - 1) / brick_size;
            };

            // A brick count of one makes the lattice collapse to the bounding box
            // itself, see sdf_bounds.hlsl.
            data.brick_count = parameter.render_bricks ?
                                   vec3u{get_brick_count(sdf_extent.width),
                                         get_brick_count(sdf_extent.height),
                                         get_brick_count(sdf_extent.depth)} :
                                   vec3u{1, 1, 1};

            data.bounding_box = parameter.bounding_box;
            data.matrix_m = parameter.matrix_m;

            // One line per brick boundary and two vertices per line. Must match
            // the vertex generation in sdf_bounds.hlsl.
            const vec3u& count = data.brick_count;
            const std::uint32_t line_count = ((count.y + 1) * (count.z + 1)) +
                                             ((count.x + 1) * (count.z + 1)) +
                                             ((count.x + 1) * (count.y + 1));
            data.vertex_count = line_count * 2;
        },
        [brick_enable = parameter.render_bricks](const pass_data& data, rdg_command& command)
        {
            command.set_viewport();
            command.set_scissor();

            auto& device = render_device::instance();

            std::vector<std::wstring> defines;
            if (brick_enable)
            {
                defines.emplace_back(L"-DSDF_BOUNDS_BRICK");
            }

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<sdf_bounds_vs>(defines),
                .fragment_shader = device.get_shader<sdf_bounds_fs>(defines),
                .rasterizer_state = device.get_rasterizer_state<RHI_CULL_MODE_NONE>(),
                // Reversed depth: a line is hidden by the SDF surface in front of
                // it. The wireframe is an overlay, so it does not write depth.
                .depth_stencil_state =
                    device.get_depth_stencil_state<true, false, RHI_COMPARE_OP_GREATER_EQUAL>(),
                .primitive_topology = RHI_PRIMITIVE_TOPOLOGY_LINE_LIST,
            };

            command.set_pipeline(pipeline);

            sdf_bounds_vs::constant_data constant = {
                .bounding_box_min = data.bounding_box.min,
                .bounding_box_max = data.bounding_box.max,
                .brick_count = data.brick_count,
                .matrix_m = data.matrix_m,
            };
            command.set_constant(constant);

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_CAMERA);

            command.draw(0, data.vertex_count);
        });
}
} // namespace violet
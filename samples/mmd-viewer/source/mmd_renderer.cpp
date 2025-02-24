#include "mmd_renderer.hpp"
#include "graphics/passes/blit_pass.hpp"
#include "graphics/passes/cull_pass.hpp"
#include "graphics/passes/gtao_pass.hpp"
#include "graphics/passes/hzb_pass.hpp"
#include "graphics/passes/lighting/unlit_pass.hpp"
#include "graphics/passes/mesh_pass.hpp"
#include "graphics/passes/motion_vector_pass.hpp"
#include "graphics/passes/skybox_pass.hpp"
#include "graphics/passes/taa_pass.hpp"
#include "graphics/passes/tone_mapping_pass.hpp"
#include "mmd_material.hpp"

namespace violet
{
struct toon_fs : public mesh_fs
{
    static constexpr std::string_view path = "assets/shaders/mmd_toon.hlsl";

    struct constant_data
    {
        std::uint32_t color;
        std::uint32_t ao_buffer;
    };

    static constexpr parameter parameter = {
        {
            .type = RHI_PARAMETER_BINDING_CONSTANT,
            .stages = RHI_SHADER_STAGE_FRAGMENT,
            .size = sizeof(constant_data),
        },
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, parameter},
    };
};

void mmd_renderer::render(render_graph& graph)
{
    m_render_extent = graph.get_camera().get_render_target()->get_extent();

    m_render_target = graph.add_texture(
        "Render Target",
        m_render_extent,
        RHI_FORMAT_R16G16B16A16_FLOAT,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        m_render_extent,
        RHI_FORMAT_D32_FLOAT_S8_UINT,
        RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_SHADER_RESOURCE);

    if (graph.get_scene().get_instance_count() != 0)
    {
        add_cull_pass(graph);
        add_opaque_pass(graph);

        if (graph.get_camera().get_feature<gtao_render_feature>())
        {
            add_hzb_pass(graph);
            add_gtao_pass(graph);
        }
        else
        {
            m_ao_buffer = nullptr;
        }

        add_lighting_pass(graph);
    }

    if (graph.get_scene().has_skybox())
    {
        add_skybox_pass(graph);
    }

    if (graph.get_scene().get_instance_count() != 0)
    {
        add_transparent_pass(graph);
    }

    if (graph.get_camera().get_feature<taa_render_feature>())
    {
        add_motion_vector_pass(graph);
        add_taa_pass(graph);
    }

    add_tone_mapping_pass(graph);
    add_present_pass(graph);

    m_imgui_pass.add(
        graph,
        {
            .render_target = m_render_target,
        });
}

void mmd_renderer::add_cull_pass(render_graph& graph)
{
    rdg_scope scope(graph, "Cull");

    m_command_buffer = graph.add_buffer(
        "Command Buffer",
        graph.get_scene().get_instance_capacity() * sizeof(shader::draw_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_count_buffer = graph.add_buffer(
        "Count Buffer",
        graph.get_scene().get_group_capacity() * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE_TEXEL | RHI_BUFFER_INDIRECT | RHI_BUFFER_TRANSFER_DST);

    cull_pass::add(
        graph,
        {
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
        });
}

void mmd_renderer::add_opaque_pass(render_graph& graph)
{
    rdg_scope scope(graph, "Opaque Mesh");

    m_gbuffer_albedo = graph.add_texture(
        "GBuffer Albedo",
        m_render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_gbuffer_material = graph.add_texture(
        "GBuffer Material",
        m_render_extent,
        RHI_FORMAT_R8G8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_gbuffer_normal = graph.add_texture(
        "GBuffer Normal",
        m_render_extent,
        RHI_FORMAT_R16G16_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    m_gbuffer_emissive = graph.add_texture(
        "GBuffer Emissive",
        m_render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_SHADER_RESOURCE);

    std::vector<rdg_texture*> render_targets = {
        m_gbuffer_albedo,
        m_gbuffer_material,
        m_gbuffer_normal,
        m_gbuffer_emissive,
    };

    mesh_pass::add(
        graph,
        {
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
            .render_targets = render_targets,
            .depth_buffer = m_depth_buffer,
            .material_type = MATERIAL_OPAQUE,
            .clear = true,
        });

    mesh_pass::add(
        graph,
        {
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
            .render_targets = render_targets,
            .depth_buffer = m_depth_buffer,
            .material_type = MATERIAL_TOON,
        });
}

void mmd_renderer::add_hzb_pass(render_graph& graph)
{
    std::uint32_t level_count =
        static_cast<std::uint32_t>(
            std::floor(std::log2(std::max(m_render_extent.width, m_render_extent.height)))) +
        1;

    m_hzb = graph.add_texture(
        "HZB",
        m_render_extent,
        RHI_FORMAT_R32_FLOAT,
        RHI_TEXTURE_SHADER_RESOURCE | RHI_TEXTURE_STORAGE,
        level_count);

    hzb_pass::add(
        graph,
        {
            .depth_buffer = m_depth_buffer,
            .hzb = m_hzb,
        });
}

void mmd_renderer::add_gtao_pass(render_graph& graph)
{
    auto* gtao = graph.get_camera().get_feature<gtao_render_feature>();

    m_ao_buffer = graph.add_texture(
        "AO Buffer",
        m_render_extent,
        RHI_FORMAT_R8_UNORM,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    gtao_pass::add(
        graph,
        {
            .slice_count = gtao->get_slice_count(),
            .step_count = gtao->get_step_count(),
            .radius = gtao->get_radius(),
            .falloff = gtao->get_falloff(),
            .depth_buffer = m_hzb,
            .normal_buffer = m_gbuffer_normal,
            .ao_buffer = m_ao_buffer,
        });
}

void mmd_renderer::add_lighting_pass(render_graph& graph)
{
    unlit_pass::add(
        graph,
        {
            .gbuffer_albedo = m_gbuffer_albedo,
            .depth_buffer = m_depth_buffer,
            .render_target = m_render_target,
            .clear = true,
        });

    struct pass_data
    {
        rdg_texture_srv gbuffer_color;
        rdg_texture_srv ao_buffer;
        rhi_parameter* toon_parameter;
    };

    graph.add_pass<pass_data>(
        "Lighting Pass",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.add_render_target(m_render_target, RHI_ATTACHMENT_LOAD_OP_LOAD);
            pass.set_depth_stencil(m_depth_buffer, RHI_ATTACHMENT_LOAD_OP_LOAD);

            data.gbuffer_color =
                pass.add_texture_srv(m_gbuffer_albedo, RHI_PIPELINE_STAGE_FRAGMENT);

            if (m_ao_buffer != nullptr)
            {
                data.ao_buffer = pass.add_texture_srv(m_ao_buffer, RHI_PIPELINE_STAGE_FRAGMENT);
            }
            else
            {
                data.ao_buffer = rdg_texture_srv();
            }

            data.toon_parameter = pass.add_parameter(toon_fs::parameter);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            toon_fs::constant_data constant = {
                .color = data.gbuffer_color.get_rhi()->get_bindless(),
            };

            std::vector<std::wstring> defines;
            if (data.ao_buffer)
            {
                defines.emplace_back(L"-DUSE_AO_BUFFER");
                constant.ao_buffer = data.ao_buffer.get_rhi()->get_bindless();
            }

            data.toon_parameter->set_constant(0, &constant, sizeof(toon_fs::constant_data));

            rdg_raster_pipeline pipeline = {
                .vertex_shader = device.get_shader<fullscreen_vs>(),
                .fragment_shader = device.get_shader<toon_fs>(defines),
            };
            pipeline.depth_stencil.stencil_enable = true;
            pipeline.depth_stencil.stencil_front = {
                .compare_op = RHI_COMPARE_OP_EQUAL,
                .reference = SHADING_MODEL_TOON,
            };
            pipeline.depth_stencil.stencil_back = pipeline.depth_stencil.stencil_front;

            command.set_pipeline(pipeline);
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, data.toon_parameter);
            command.draw_fullscreen();
        });
}

void mmd_renderer::add_skybox_pass(render_graph& graph)
{
    skybox_pass::add(
        graph,
        {
            .render_target = m_render_target,
            .depth_buffer = m_depth_buffer,
            .clear = graph.get_scene().get_instance_count() == 0,
        });
}

void mmd_renderer::add_transparent_pass(render_graph& graph)
{
    std::vector<rdg_texture*> render_targets = {
        m_render_target,
    };

    mesh_pass::add(
        graph,
        {
            .command_buffer = m_command_buffer,
            .count_buffer = m_count_buffer,
            .render_targets = render_targets,
            .depth_buffer = m_depth_buffer,
            .material_type = MATERIAL_TRANSPARENT,
        });
}

void mmd_renderer::add_motion_vector_pass(render_graph& graph)
{
    m_motion_vectors = graph.add_texture(
        "Motion Vectors",
        m_render_extent,
        RHI_FORMAT_R16G16_FLOAT,
        RHI_TEXTURE_STORAGE | RHI_TEXTURE_SHADER_RESOURCE);

    motion_vector_pass::add(
        graph,
        {
            .depth_buffer = m_depth_buffer,
            .motion_vector = m_motion_vectors,
        });
}

void mmd_renderer::add_taa_pass(render_graph& graph)
{
    auto* taa = graph.get_camera().get_feature<taa_render_feature>();

    rdg_texture* resolved_render_target = graph.add_texture(
        "Resolved Render Target",
        taa->get_current(),
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    rdg_texture* history_render_target = nullptr;
    if (taa->is_history_valid())
    {
        history_render_target = graph.add_texture(
            "History Render Target",
            taa->get_history(),
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
            RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);
    }

    taa_pass::add(
        graph,
        {
            .current_render_target = m_render_target,
            .history_render_target = history_render_target,
            .depth_buffer = m_depth_buffer,
            .motion_vector = m_motion_vectors,
            .resolved_render_target = resolved_render_target,
        });

    m_render_target = resolved_render_target;
}

void mmd_renderer::add_tone_mapping_pass(render_graph& graph)
{
    rdg_texture* ldr_target = graph.add_texture(
        "LDR Target",
        m_render_extent,
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_TRANSFER_SRC | RHI_TEXTURE_STORAGE);

    tone_mapping_pass::add(
        graph,
        {
            .hdr_texture = m_render_target,
            .ldr_texture = ldr_target,
        });

    m_render_target = ldr_target;
}

void mmd_renderer::add_present_pass(render_graph& graph)
{
    rdg_texture* camera_output = graph.add_texture(
        "Camera Output",
        graph.get_camera().get_render_target(),
        RHI_TEXTURE_LAYOUT_UNDEFINED,
        RHI_TEXTURE_LAYOUT_PRESENT);

    rhi_texture_region region = {
        .offset_x = 0,
        .offset_y = 0,
        .extent = m_render_extent,
        .level = 0,
        .layer = 0,
        .layer_count = 1,
    };

    blit_pass::add(
        graph,
        {
            .src = m_render_target,
            .src_region = region,
            .dst = camera_output,
            .dst_region = region,
        });

    m_render_target = camera_output;
}
} // namespace violet
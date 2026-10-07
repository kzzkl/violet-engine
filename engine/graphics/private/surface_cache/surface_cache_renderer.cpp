#include "surface_cache/surface_cache_renderer.hpp"
#include "common/utility.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "math/box.hpp"
#include <format>

namespace violet
{
struct surface_cache_face
{
    vec3f direction;
    vec3f up;
};

constexpr surface_cache_face SURFACE_CACHE_FACES[SURFACE_CACHE_FACE_COUNT] = {
    {.direction = {1.0f, 0.0f, 0.0f}, .up = {0.0f, 1.0f, 0.0f}},
    {.direction = {-1.0f, 0.0f, 0.0f}, .up = {0.0f, 1.0f, 0.0f}},
    {.direction = {0.0f, 1.0f, 0.0f}, .up = {0.0f, 0.0f, -1.0f}},
    {.direction = {0.0f, -1.0f, 0.0f}, .up = {0.0f, 0.0f, 1.0f}},
    {.direction = {0.0f, 0.0f, 1.0f}, .up = {0.0f, 1.0f, 0.0f}},
    {.direction = {0.0f, 0.0f, -1.0f}, .up = {0.0f, 1.0f, 0.0f}},
};

struct surface_cache_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/surface_cache/surface_cache.hlsl";

    struct constant_data
    {
        mat4f matrix_vp;
        std::uint32_t submesh_id;
        std::uint32_t geometry_buffer;

        std::uint32_t vertex_buffer;

        std::uint32_t material_buffer;
        std::uint32_t material_address;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct surface_cache_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/surface_cache/surface_cache.hlsl";

    using constant_data = surface_cache_vs::constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

void surface_cache_renderer::render(
    render_graph& graph,
    std::span<surface_cache_render_item> items,
    rhi_texture* albedo_buffer,
    rhi_texture* depth_buffer)
{
    m_albedo_temp = graph.add_texture(
        "Albedo Buffer Temp",
        rhi_extent{
            .width = 512,
            .height = 512,
            .depth = 1,
        },
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_TRANSFER_SRC);

    m_depth_temp = graph.add_texture(
        "Depth Buffer Temp",
        rhi_extent{
            .width = 512,
            .height = 512,
            .depth = 1,
        },
        RHI_FORMAT_D32_FLOAT,
        RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_TRANSFER_SRC);

    m_albedo_buffer = graph.add_texture(
        "Albedo Buffer",
        albedo_buffer,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        depth_buffer,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
        RHI_TEXTURE_LAYOUT_SHADER_RESOURCE);

    for (const auto& item : items)
    {
        for (std::uint32_t i = 0; i < SURFACE_CACHE_FACE_COUNT; ++i)
        {
            bool clear = true;

            for (const auto& [submesh_index, material] : item.materials)
            {
                render_surface(
                    graph,
                    item.geometry,
                    material,
                    submesh_index,
                    i,
                    item.faces[i].width,
                    item.faces[i].height,
                    clear);
                clear = false;
            }

            copy_surface(graph, item.faces[i]);
        }
    }
}

void surface_cache_renderer::render_surface(
    render_graph& graph,
    geometry* geometry,
    material* material,
    std::uint32_t submesh_index,
    std::uint32_t face,
    std::uint32_t width,
    std::uint32_t height,
    bool clear)
{
    auto& device = render_device::instance();

    const box3f& volume_bounds = geometry->get_distance_field().volume_bounds;

    vec3f volume_center = box::get_center(volume_bounds);
    float volume_extent = vector::max(box::get_extent(volume_bounds));

    const surface_cache_face& face_data = SURFACE_CACHE_FACES[face];

    mat4f matrix_v = matrix::look_at(
        volume_center + face_data.direction * volume_extent * 0.5f,
        volume_center,
        face_data.up);
    mat4f matrix_p = matrix::orthographic(volume_extent, volume_extent, 0.0f, 100.0f);

    mat4f matrix_vp = matrix::mul(matrix_v, matrix_p);

    std::vector<std::wstring> defines;
    defines.emplace_back(
        std::format(L"-DMATERIAL_NAME={}", string_to_wstring(material->get_name())));
    defines.emplace_back(
        std::format(L"-DMATERIAL_SHADER=\"{}\"", string_to_wstring(material->get_shader_path())));

    rdg_raster_pipeline pipeline = {
        .vertex_shader = device.get_shader<surface_cache_vs>(defines),
        .fragment_shader = device.get_shader<surface_cache_fs>(defines),
        .rasterizer_state =
            device.get_rasterizer_state<RHI_CULL_MODE_BACK, RHI_POLYGON_MODE_FILL>(),
        .depth_stencil_state = device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_LESS>(),
        .primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    struct pass_data
    {
        mat4f matrix_vp;

        std::uint32_t submesh_id;
        const geometry::submesh* submesh;

        std::uint32_t index_offset;

        std::uint32_t geometry_buffer;
        std::uint32_t vertex_buffer;

        std::uint32_t material_buffer;
        std::uint32_t material_address;

        rdg_raster_pipeline pipeline;
    };

    graph.add_pass<pass_data>(
        "Surface Cache",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            rhi_attachment_load_op load_op =
                clear ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;

            pass.add_render_target(m_albedo_temp, load_op);
            pass.set_depth_stencil(
                m_depth_temp,
                load_op,
                RHI_ATTACHMENT_STORE_OP_STORE,
                0,
                0,
                {
                    .depth_stencil = {.depth = 1.0f, .stencil = 0},
                });

            auto* material_manager = device.get_material_manager();
            auto* geometry_manager = device.get_geometry_manager();

            data.matrix_vp = matrix_vp;

            data.submesh_id = geometry->get_submesh_id(submesh_index);

            data.submesh = &geometry->get_submesh(submesh_index);

            data.index_offset = geometry_manager->get_buffer_address(
                                    geometry->get_geometry_id(),
                                    GEOMETRY_BUFFER_INDEX) /
                                4;

            data.geometry_buffer =
                geometry_manager->get_geometry_buffer()->get_srv()->get_bindless();
            data.vertex_buffer = geometry_manager->get_vertex_buffer()->get_srv()->get_bindless();

            data.material_buffer =
                material_manager->get_material_buffer()->get_srv()->get_bindless();
            data.material_address =
                material_manager->get_material_constant_address(material->get_material_id());

            data.pipeline = pipeline;
        },
        [width, height](const pass_data& data, rdg_command& command)
        {
            command.set_pipeline(data.pipeline);

            command.set_constant(
                surface_cache_vs::constant_data{
                    .matrix_vp = data.matrix_vp,
                    .submesh_id = data.submesh_id,
                    .geometry_buffer = data.geometry_buffer,
                    .vertex_buffer = data.vertex_buffer,
                    .material_buffer = data.material_buffer,
                    .material_address = data.material_address,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.set_index_buffer();

            command.set_viewport({
                .width = static_cast<float>(width),
                .height = static_cast<float>(height),
                .min_depth = 0.0f,
                .max_depth = 1.0f,
            });

            rhi_scissor_rect scissor_rect = {
                .min_x = 0,
                .min_y = 0,
                .max_x = width,
                .max_y = height,
            };
            command.set_scissor(std::span(&scissor_rect, 1));

            if (data.submesh->has_cluster())
            {
                for (const auto& cluster : data.submesh->clusters)
                {
                    if (cluster.lod != 0)
                    {
                        break;
                    }

                    command.draw_indexed(
                        cluster.index_offset + data.index_offset,
                        cluster.index_count,
                        0);
                }
            }
            else
            {
                command.draw_indexed(
                    data.submesh->index_offset + data.index_offset,
                    data.submesh->index_count,
                    data.submesh->vertex_offset);
            }
        });
}

void surface_cache_renderer::copy_surface(
    render_graph& graph,
    const surface_cache_render_item::face& face)
{
    struct pass_data
    {
        rdg_texture_ref albedo_temp;
        rdg_texture_ref depth_temp;

        rdg_texture_ref albedo_buffer;
        rdg_texture_ref depth_buffer;
    };

    graph.add_pass<pass_data>(
        "Surface Cache Copy",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.albedo_temp = pass.add_texture(
                m_albedo_temp,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_READ,
                RHI_TEXTURE_LAYOUT_TRANSFER_SRC);
            data.depth_temp = pass.add_texture(
                m_depth_temp,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_READ,
                RHI_TEXTURE_LAYOUT_TRANSFER_SRC);

            data.albedo_buffer = pass.add_texture(
                m_albedo_buffer,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE,
                RHI_TEXTURE_LAYOUT_TRANSFER_DST);
            data.depth_buffer = pass.add_texture(
                m_depth_buffer,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE,
                RHI_TEXTURE_LAYOUT_TRANSFER_DST);
        },
        [face](const pass_data& data, rdg_command& command)
        {
            rhi_texture_region src_region = {
                .level = 0,
                .layer = 0,
                .layer_count = 1,
            };

            rhi_texture_region dst_region = {
                .level = 0,
                .layer = 0,
                .layer_count = 1,
            };

            for (std::size_t i = 0; i < face.page_src_coords.size(); ++i)
            {
                src_region.offset_x = static_cast<std::int32_t>(face.page_src_coords[i].x);
                src_region.offset_y = static_cast<std::int32_t>(face.page_src_coords[i].y);
                src_region.aspect = RHI_TEXTURE_ASPECT_COLOR;

                dst_region.offset_x = static_cast<std::int32_t>(face.page_dst_coords[i].x);
                dst_region.offset_y = static_cast<std::int32_t>(face.page_dst_coords[i].y);
                dst_region.aspect = RHI_TEXTURE_ASPECT_COLOR;

                src_region.extent = dst_region.extent = {
                    .width = face.page_extents[i].x,
                    .height = face.page_extents[i].y,
                    .depth = 1,
                };

                command.copy_texture(
                    data.albedo_temp.get_rhi(),
                    src_region,
                    data.albedo_buffer.get_rhi(),
                    dst_region);

                src_region.aspect = RHI_TEXTURE_ASPECT_DEPTH;
                dst_region.aspect = RHI_TEXTURE_ASPECT_DEPTH;

                command.copy_texture(
                    data.depth_temp.get_rhi(),
                    src_region,
                    data.depth_buffer.get_rhi(),
                    dst_region);
            }
        });
}
} // namespace violet
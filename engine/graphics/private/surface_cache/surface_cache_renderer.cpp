#include "surface_cache/surface_cache_renderer.hpp"
#include "common/utility.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/material_manager.hpp"
#include "math/box.hpp"
#include <format>

namespace violet
{
constexpr std::uint32_t SURFACE_CACHE_FACE_COUNT = 6;

// The surface cache is built by looking at the volume bounds of the geometry from six directions.
// The volume bounds are in geometry space, which is the space the vertex shader renders in (its
// model matrix is identity), and the SDF maps them to a cube whose side is the largest extent of
// the bounds (see distance_field_manager and render_scene_sdf), so every face is rendered
// orthographically over that cube. The face order and the up vectors follow the cube map
// convention of the graphics API, so the result can be sampled by face index.
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
    static constexpr std::string_view path = "assets/shaders/surface_cache.hlsl";

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
    static constexpr std::string_view path = "assets/shaders/surface_cache.hlsl";

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
    m_albedo_buffer = graph.add_texture(
        "Albedo Buffer",
        rhi_extent{
            .width = 512,
            .height = 512,
            .depth = 1,
        },
        RHI_FORMAT_R8G8B8A8_UNORM,
        RHI_TEXTURE_RENDER_TARGET | RHI_TEXTURE_TRANSFER_SRC);

    m_depth_buffer = graph.add_texture(
        "Depth Buffer",
        rhi_extent{
            .width = 512,
            .height = 512,
            .depth = 1,
        },
        RHI_FORMAT_D32_FLOAT,
        RHI_TEXTURE_DEPTH_STENCIL | RHI_TEXTURE_TRANSFER_SRC);

    for (const auto& item : items)
    {
        for (std::uint32_t i = 0; i < SURFACE_CACHE_FACE_COUNT; ++i)
        {
            for (const auto& [submesh_index, material] : item.materials)
            {
                render_surface(graph, item.geometry, material, submesh_index, i);
            }

            copy_surface(graph, item);
        }
    }
}

void surface_cache_renderer::render_surface(
    render_graph& graph,
    geometry* geometry,
    material* material,
    std::uint32_t submesh_index,
    std::uint32_t face)
{
    auto& device = render_device::instance();

    // Render the volume bounds of the geometry from outside one of its six faces. Because the
    // volume is a cube whose side is the largest extent of the bounds, the orthographic projection
    // covers the whole face and the six faces have the same size.
    const box3f& volume_bounds = geometry->get_distance_field().volume_bounds;

    vec3f volume_center = box::get_center(volume_bounds);
    float volume_extent = vector::max(box::get_extent(volume_bounds));

    const surface_cache_face& face_data = SURFACE_CACHE_FACES[face];

    // Place the view one extent away from the volume, so the near and far planes span the volume.
    float view_distance = volume_extent;

    mat4f matrix_v = matrix::look_at(
        volume_center + face_data.direction * view_distance,
        volume_center,
        face_data.up);

    // The engine uses reversed depth (see camera_system), so the farther plane is passed as the
    // near plane and the nearer plane as the far plane.
    mat4f matrix_p = matrix::orthographic(128, 128, 0.0f, 100.0f);

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
        .depth_stencil_state = device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
        .primitive_topology = RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    };

    struct pass_data
    {
        mat4f matrix_vp;

        std::uint32_t submesh_id;
        std::uint32_t vertex_offset;
        std::uint32_t index_offset;
        std::uint32_t index_count;

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
                face == 0 ? RHI_ATTACHMENT_LOAD_OP_CLEAR : RHI_ATTACHMENT_LOAD_OP_LOAD;

            pass.add_render_target(m_albedo_buffer, load_op);
            pass.set_depth_stencil(m_depth_buffer, load_op);

            auto* material_manager = device.get_material_manager();
            auto* geometry_manager = device.get_geometry_manager();

            data.matrix_vp = matrix_vp;

            data.submesh_id = geometry->get_submesh_id(submesh_index);

            const auto& submesh = geometry->get_submesh(submesh_index);
            data.vertex_offset = submesh.vertex_offset;
            data.index_offset = submesh.index_offset;
            data.index_count = submesh.index_count;

            data.vertex_buffer = geometry_manager->get_vertex_buffer()->get_srv()->get_bindless();

            data.material_buffer =
                material_manager->get_material_buffer()->get_srv()->get_bindless();
            data.material_address =
                material_manager->get_material_constant_address(material->get_material_id());

            data.pipeline = pipeline;
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.set_pipeline(data.pipeline);

            command.set_constant(
                surface_cache_vs::constant_data{
                    .matrix_vp = data.matrix_vp,
                    .submesh_id = data.submesh_id,
                    .geometry_buffer = data.vertex_buffer,
                    .vertex_buffer = data.vertex_buffer,
                    .material_buffer = data.material_buffer,
                    .material_address = data.material_address,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.set_index_buffer();

            command.set_viewport({
                .width = 128,
                .height = 128,
                .min_depth = 0.0f,
                .max_depth = 1.0f,
            });

            rhi_scissor_rect scissor_rect = {
                .min_x = 0,
                .min_y = 0,
                .max_x = 128,
                .max_y = 128,
            };
            command.set_scissor(std::span(&scissor_rect, 1));

            // command.draw_indexed(data.index_offset, data.index_count, data.vertex_offset);
        });
}

void surface_cache_renderer::copy_surface(
    render_graph& graph,
    const surface_cache_render_item& item)
{
}
} // namespace violet
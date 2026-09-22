#include "graphics/renderers/passes/sdf_pass.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/render_scene/render_scene_sdf.hpp"

namespace violet
{
struct sdf_clear_clipmap_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/clear_clipmap.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_state;
        std::uint32_t page_table;
        std::uint32_t free_pages;
        std::uint32_t page_atlas_capacity;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_prepare_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/prepare.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_state;
        std::uint32_t clipmap_level_mesh_counts;
        std::uint32_t invalidated_grid_indirect_args;
        std::uint32_t invalidated_page_indirect_args;
        std::uint32_t pages_to_allocate_indirect_args;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_mark_dirty_pages_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/mark_dirty_pages.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_state;

        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        std::uint32_t invalidation_regions;
        std::uint32_t invalidation_region_count;

        std::uint32_t invalidated_grids;
        std::uint32_t invalidated_grid_indirect_args;
        std::uint32_t invalidated_pages;
        std::uint32_t invalidated_page_indirect_args;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_mesh_classify_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/mesh_classify.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        std::uint32_t mesh_buffer;
        std::uint32_t mesh_count;

        std::uint32_t clipmap_level_meshes;
        std::uint32_t clipmap_level_mesh_counts;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_mesh_grid_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/mesh_grid_cull.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_state;

        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        std::uint32_t invalidated_grids;

        std::uint32_t mesh_buffer;

        std::uint32_t clipmap_level_meshes;
        std::uint32_t clipmap_level_mesh_counts;

        std::uint32_t invalidated_grid_meshes;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_mesh_page_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/mesh_page_cull.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_state;

        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        std::uint32_t invalidated_grids;
        std::uint32_t invalidated_pages;
        std::uint32_t invalidated_grid_meshes;

        std::uint32_t mesh_buffer;

        std::uint32_t page_table;
        std::uint32_t free_pages;

        std::uint32_t pages_to_allocate;
        std::uint32_t pages_to_allocate_indirect_args;

        std::uint32_t pages_to_update;
        std::uint32_t pages_to_update_indirect_args;

        std::uint32_t page_atlas_capacity;

        std::uint32_t distance_field_buffer;
        std::uint32_t brick_table;
        std::uint32_t brick_atlas;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_allocate_pages_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/allocate_pages.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_state;

        std::uint32_t page_table;
        std::uint32_t free_pages;

        std::uint32_t pages_to_allocate;

        std::uint32_t pages_to_update;
        std::uint32_t pages_to_update_indirect_args;

        std::uint32_t page_atlas_capacity;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_update_pages_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/update_pages.hlsl";

    struct constant_data
    {
        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        std::uint32_t invalidated_grids;
        std::uint32_t invalidated_grid_meshes;

        std::uint32_t mesh_buffer;

        std::uint32_t page_table;
        std::uint32_t page_atlas;

        std::uint32_t pages_to_update;

        std::uint32_t distance_field_buffer;
        std::uint32_t brick_table;
        std::uint32_t brick_atlas;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct sdf_debug_page_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/distance_field/sdf_debug_page.hlsl";

    struct constant_data
    {
        std::uint32_t debug_output;
        std::uint32_t depth_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

struct sdf_debug_mesh_sdf_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/distance_field/sdf_debug_mesh_sdf.hlsl";

    struct constant_data
    {
        std::uint32_t mesh_index;
        std::uint32_t mesh_buffer;

        std::uint32_t distance_field_buffer;
        std::uint32_t brick_table;
        std::uint32_t brick_atlas;

        std::uint32_t debug_output;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void sdf_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "SDF");

    const auto& context = graph.get_context();
    const auto& sdf_module = context.get_module<render_scene_sdf>();

    m_mesh_buffer = graph.add_buffer("SDF Mesh Buffer", sdf_module.get_mesh_buffer());
    m_mesh_count = sdf_module.get_mesh_count();

    auto clipmap = sdf_module.get_clipmap(context.get_camera().id);

    m_clipmap_state = graph.add_buffer("SDF Clipmap State", clipmap.state);

    m_clipmap_levels = sdf_module.get_clipmap_levels_buffer()->get_srv()->get_bindless();
    m_clipmap_level_offset = clipmap.level_offset;

    m_page_table = graph.add_texture("SDF Page Table", clipmap.page_table);
    m_page_atlas = graph.add_texture("SDF Page Atlas", clipmap.page_atlas);
    m_free_pages = graph.add_buffer("SDF Free Pages", clipmap.free_pages);
    m_page_atlas_capacity = clipmap.page_atlas_capacity;

    m_invalidated_grids = graph.add_buffer(
        "SDF Invalidated Grids",
        sizeof(vec2u) * SDF_CLIPMAP_GRID_COUNT * SDF_CLIPMAP_LEVEL_COUNT,
        RHI_BUFFER_STORAGE);
    m_invalidated_grid_indirect_args = graph.add_buffer(
        "SDF Invalidated Grid Indirect Args",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_invalidated_pages = graph.add_buffer(
        "SDF Invalidated Pages",
        sizeof(std::uint32_t) * SDF_CLIPMAP_PAGE_COUNT * SDF_CLIPMAP_LEVEL_COUNT,
        RHI_BUFFER_STORAGE);
    m_invalidated_page_indirect_args = graph.add_buffer(
        "SDF Invalidated Page Indirect Args",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_clipmap_level_meshes = graph.add_buffer(
        "SDF Clipmap Level Meshes",
        sizeof(std::uint32_t) * SDF_MAX_MESH_COUNT * SDF_CLIPMAP_LEVEL_COUNT,
        RHI_BUFFER_STORAGE);
    m_clipmap_level_mesh_counts = graph.add_buffer(
        "SDF Clipmap Level Mesh Counts",
        sizeof(std::uint32_t) * SDF_CLIPMAP_LEVEL_COUNT,
        RHI_BUFFER_STORAGE);

    m_invalidated_grid_meshes = graph.add_buffer(
        "SDF Invalidated Grid Meshes",
        sizeof(std::uint32_t) * SDF_CLIPMAP_GRID_COUNT * SDF_CLIPMAP_AVERAGE_MESH_COUNT_PER_GRID *
            SDF_CLIPMAP_LEVEL_COUNT,
        RHI_BUFFER_STORAGE);

    m_pages_to_allocate = graph.add_buffer(
        "SDF Pages to Allocate",
        sizeof(std::uint32_t) * m_page_atlas_capacity,
        RHI_BUFFER_STORAGE);
    m_pages_to_allocate_indirect_args = graph.add_buffer(
        "SDF Pages to Allocate Indirect Args",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_pages_to_update = graph.add_buffer(
        "SDF Pages to Update",
        sizeof(std::uint32_t) * m_page_atlas_capacity,
        RHI_BUFFER_STORAGE);
    m_pages_to_update_indirect_args = graph.add_buffer(
        "SDF Pages to Update Indirect Args",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    auto& device = render_device::instance();

    auto* geometry_manager = device.get_geometry_manager();
    m_distance_field_buffer =
        geometry_manager->get_distance_field_buffer()->get_srv()->get_bindless();
    m_brick_table = geometry_manager->get_distance_field_brick_table()->get_srv()->get_bindless();
    m_brick_atlas = geometry_manager->get_distance_field_brick_atlas()
                        ->get_srv(RHI_TEXTURE_DIMENSION_3D)
                        ->get_bindless();

    if (clipmap.need_clear)
    {
        initialize_clipmaps(graph);
    }

    prepare(graph);

    mark_invalidated_page(graph);
    mesh_classify(graph);
    mesh_grid_cull(graph);
    mesh_page_cull(graph);

    allocate_pages(graph);
    update_pages(graph);

    if (parameter.debug_mode != DEBUG_MODE_NONE)
    {
        add_debug_pass(graph, parameter);
    }
}

void sdf_pass::initialize_clipmaps(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav clipmap_state;
        rdg_texture_uav page_table;
        rdg_buffer_uav free_pages;
        std::uint32_t page_atlas_capacity;
    };

    graph.add_pass<pass_data>(
        "SDF Clear Page Table",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.clipmap_state = pass.add_buffer_uav(m_clipmap_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.page_table = pass.add_texture_uav(
                m_page_table,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_3D);
            data.free_pages = pass.add_buffer_uav(m_free_pages, RHI_PIPELINE_STAGE_COMPUTE);
            data.page_atlas_capacity = m_page_atlas_capacity;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_clear_clipmap_cs>(),
            });

            command.set_constant(
                sdf_clear_clipmap_cs::constant_data{
                    .clipmap_state = data.clipmap_state.get_bindless(),
                    .page_table = data.page_table.get_bindless(),
                    .free_pages = data.free_pages.get_bindless(),
                    .page_atlas_capacity = data.page_atlas_capacity,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_3d(
                SDF_CLIPMAP_PAGE_COUNT_PER_AXIS,
                SDF_CLIPMAP_PAGE_COUNT_PER_AXIS,
                SDF_CLIPMAP_PAGE_COUNT_PER_AXIS * SDF_CLIPMAP_LEVEL_COUNT);
        });
}

void sdf_pass::prepare(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav clipmap_state;
        rdg_buffer_uav clipmap_level_mesh_counts;

        rdg_buffer_uav invalidated_grid_indirect_args;
        rdg_buffer_uav invalidated_page_indirect_args;

        rdg_buffer_uav pages_to_allocate_indirect_args;
    };

    graph.add_pass<pass_data>(
        "SDF Prepare",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.clipmap_state = pass.add_buffer_uav(m_clipmap_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.clipmap_level_mesh_counts =
                pass.add_buffer_uav(m_clipmap_level_mesh_counts, RHI_PIPELINE_STAGE_COMPUTE);

            data.invalidated_grid_indirect_args =
                pass.add_buffer_uav(m_invalidated_grid_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidated_page_indirect_args =
                pass.add_buffer_uav(m_invalidated_page_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);

            data.pages_to_allocate_indirect_args =
                pass.add_buffer_uav(m_pages_to_allocate_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_prepare_cs>(),
            });

            command.set_constant(
                sdf_prepare_cs::constant_data{
                    .clipmap_state = data.clipmap_state.get_bindless(),
                    .clipmap_level_mesh_counts = data.clipmap_level_mesh_counts.get_bindless(),
                    .invalidated_grid_indirect_args =
                        data.invalidated_grid_indirect_args.get_bindless(),
                    .invalidated_page_indirect_args =
                        data.invalidated_page_indirect_args.get_bindless(),
                    .pages_to_allocate_indirect_args =
                        data.pages_to_allocate_indirect_args.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(1, 1);
        });
}

void sdf_pass::mark_invalidated_page(render_graph& graph)
{
    const auto& context = graph.get_context();
    const auto& sdf_module = context.get_module<render_scene_sdf>();

    if (sdf_module.get_invalidation_region_count() == 0)
    {
        return;
    }

    rdg_buffer* invalidation_regions =
        graph.add_buffer("SDF Invalidation Regions", sdf_module.get_invalidation_regions_buffer());

    struct pass_data
    {
        rdg_buffer_uav clipmap_state;

        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        rdg_buffer_srv invalidation_regions;
        std::uint32_t invalidation_region_count;

        rdg_buffer_uav invalidated_grids;
        rdg_buffer_uav invalidated_grid_indirect_args;

        rdg_buffer_uav invalidated_pages;
        rdg_buffer_uav invalidated_page_indirect_args;

        vec3f camera_position;
    };

    graph.add_pass<pass_data>(
        "SDF Mark Dirty Page",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.clipmap_state = pass.add_buffer_uav(m_clipmap_state, RHI_PIPELINE_STAGE_COMPUTE);

            data.clipmap_levels = m_clipmap_levels;
            data.clipmap_level_offset = m_clipmap_level_offset;

            data.invalidation_regions =
                pass.add_buffer_srv(invalidation_regions, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidation_region_count = sdf_module.get_invalidation_region_count();

            data.invalidated_grids =
                pass.add_buffer_uav(m_invalidated_grids, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidated_grid_indirect_args =
                pass.add_buffer_uav(m_invalidated_grid_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);

            data.invalidated_pages =
                pass.add_buffer_uav(m_invalidated_pages, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidated_page_indirect_args =
                pass.add_buffer_uav(m_invalidated_page_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);

            data.camera_position = context.get_camera().position;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_mark_dirty_pages_cs>(),
            });

            command.set_constant(
                sdf_mark_dirty_pages_cs::constant_data{
                    .clipmap_state = data.clipmap_state.get_bindless(),
                    .clipmap_levels = data.clipmap_levels,
                    .clipmap_level_offset = data.clipmap_level_offset,
                    .invalidation_regions = data.invalidation_regions.get_bindless(),
                    .invalidation_region_count = data.invalidation_region_count,
                    .invalidated_grids = data.invalidated_grids.get_bindless(),
                    .invalidated_grid_indirect_args =
                        data.invalidated_grid_indirect_args.get_bindless(),
                    .invalidated_pages = data.invalidated_pages.get_bindless(),
                    .invalidated_page_indirect_args =
                        data.invalidated_page_indirect_args.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_3d(
                SDF_CLIPMAP_PAGE_COUNT_PER_AXIS,
                SDF_CLIPMAP_PAGE_COUNT_PER_AXIS,
                SDF_CLIPMAP_PAGE_COUNT_PER_AXIS * SDF_CLIPMAP_LEVEL_COUNT);
        });
}

void sdf_pass::mesh_classify(render_graph& graph)
{
    struct pass_data
    {
        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        rdg_buffer_srv mesh_buffer;
        std::uint32_t mesh_count;

        rdg_buffer_uav clipmap_level_meshes;
        rdg_buffer_uav clipmap_level_mesh_counts;

        vec3f camera_position;
    };

    graph.add_pass<pass_data>(
        "SDF Mesh Classify",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.clipmap_levels = m_clipmap_levels;
            data.clipmap_level_offset = m_clipmap_level_offset;

            data.mesh_buffer = pass.add_buffer_srv(m_mesh_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.mesh_count = m_mesh_count;

            data.clipmap_level_meshes =
                pass.add_buffer_uav(m_clipmap_level_meshes, RHI_PIPELINE_STAGE_COMPUTE);
            data.clipmap_level_mesh_counts =
                pass.add_buffer_uav(m_clipmap_level_mesh_counts, RHI_PIPELINE_STAGE_COMPUTE);

            data.camera_position = graph.get_context().get_camera().position;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_mesh_classify_cs>(),
            });

            command.set_constant(
                sdf_mesh_classify_cs::constant_data{
                    .clipmap_levels = data.clipmap_levels,
                    .clipmap_level_offset = data.clipmap_level_offset,
                    .mesh_buffer = data.mesh_buffer.get_bindless(),
                    .mesh_count = data.mesh_count,
                    .clipmap_level_meshes = data.clipmap_level_meshes.get_bindless(),
                    .clipmap_level_mesh_counts = data.clipmap_level_mesh_counts.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(data.mesh_count);
        });
}

void sdf_pass::mesh_grid_cull(render_graph& graph)
{
    struct calculate_offset_pass_data
    {
        rdg_buffer_uav clipmap_state;

        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        rdg_buffer_uav invalidated_grids;

        rdg_buffer_srv mesh_buffer;

        rdg_buffer_srv clipmap_level_meshes;
        rdg_buffer_srv clipmap_level_mesh_counts;

        rdg_buffer_ref invalidated_grid_indirect_args;
    };

    graph.add_pass<calculate_offset_pass_data>(
        "SDF Mesh Grid Cull: Calculate Offsets",
        RDG_PASS_COMPUTE,
        [&](calculate_offset_pass_data& data, rdg_pass& pass)
        {
            data.clipmap_state = pass.add_buffer_uav(m_clipmap_state, RHI_PIPELINE_STAGE_COMPUTE);

            data.clipmap_levels = m_clipmap_levels;
            data.clipmap_level_offset = m_clipmap_level_offset;

            data.invalidated_grids =
                pass.add_buffer_uav(m_invalidated_grids, RHI_PIPELINE_STAGE_COMPUTE);
            data.mesh_buffer = pass.add_buffer_srv(m_mesh_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.clipmap_level_meshes =
                pass.add_buffer_srv(m_clipmap_level_meshes, RHI_PIPELINE_STAGE_COMPUTE);
            data.clipmap_level_mesh_counts =
                pass.add_buffer_srv(m_clipmap_level_mesh_counts, RHI_PIPELINE_STAGE_COMPUTE);

            data.invalidated_grid_indirect_args = pass.add_buffer(
                m_invalidated_grid_indirect_args,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const calculate_offset_pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines = {
                L"-DCALCULATE_GRID_MESH_LIST_OFFSET",
            };

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_mesh_grid_cull_cs>(defines),
            });

            command.set_constant(
                sdf_mesh_grid_cull_cs::constant_data{
                    .clipmap_state = data.clipmap_state.get_bindless(),
                    .clipmap_levels = data.clipmap_levels,
                    .clipmap_level_offset = data.clipmap_level_offset,
                    .invalidated_grids = data.invalidated_grids.get_bindless(),
                    .mesh_buffer = data.mesh_buffer.get_bindless(),
                    .clipmap_level_meshes = data.clipmap_level_meshes.get_bindless(),
                    .clipmap_level_mesh_counts = data.clipmap_level_mesh_counts.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.invalidated_grid_indirect_args.get_rhi());
        });

    struct build_list_pass_data
    {
        rdg_buffer_srv clipmap_state;

        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        rdg_buffer_srv invalidated_grids;

        rdg_buffer_srv mesh_buffer;

        rdg_buffer_srv clipmap_level_meshes;
        rdg_buffer_srv clipmap_level_mesh_counts;

        rdg_buffer_uav invalidated_grid_meshes;

        rdg_buffer_ref invalidated_grid_indirect_args;
    };

    graph.add_pass<build_list_pass_data>(
        "SDF Mesh Grid Cull: Build Grid Mesh List",
        RDG_PASS_COMPUTE,
        [&](build_list_pass_data& data, rdg_pass& pass)
        {
            data.clipmap_state = pass.add_buffer_srv(m_clipmap_state, RHI_PIPELINE_STAGE_COMPUTE);

            data.clipmap_levels = m_clipmap_levels;
            data.clipmap_level_offset = m_clipmap_level_offset;

            data.invalidated_grids =
                pass.add_buffer_srv(m_invalidated_grids, RHI_PIPELINE_STAGE_COMPUTE);
            data.mesh_buffer = pass.add_buffer_srv(m_mesh_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.clipmap_level_meshes =
                pass.add_buffer_srv(m_clipmap_level_meshes, RHI_PIPELINE_STAGE_COMPUTE);
            data.clipmap_level_mesh_counts =
                pass.add_buffer_srv(m_clipmap_level_mesh_counts, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidated_grid_meshes =
                pass.add_buffer_uav(m_invalidated_grid_meshes, RHI_PIPELINE_STAGE_COMPUTE);

            data.invalidated_grid_indirect_args = pass.add_buffer(
                m_invalidated_grid_indirect_args,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const build_list_pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_mesh_grid_cull_cs>(),
            });

            command.set_constant(
                sdf_mesh_grid_cull_cs::constant_data{
                    .clipmap_state = data.clipmap_state.get_bindless(),
                    .clipmap_levels = data.clipmap_levels,
                    .clipmap_level_offset = data.clipmap_level_offset,
                    .invalidated_grids = data.invalidated_grids.get_bindless(),
                    .mesh_buffer = data.mesh_buffer.get_bindless(),
                    .clipmap_level_meshes = data.clipmap_level_meshes.get_bindless(),
                    .clipmap_level_mesh_counts = data.clipmap_level_mesh_counts.get_bindless(),
                    .invalidated_grid_meshes = data.invalidated_grid_meshes.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.invalidated_grid_indirect_args.get_rhi());
        });
}

void sdf_pass::mesh_page_cull(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav clipmap_state;

        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        rdg_buffer_srv invalidated_grids;
        rdg_buffer_srv invalidated_pages;
        rdg_buffer_srv invalidated_grid_meshes;
        rdg_buffer_srv mesh_buffer;

        rdg_texture_uav page_table;
        rdg_buffer_uav free_pages;

        rdg_buffer_uav pages_to_allocate;
        rdg_buffer_uav pages_to_allocate_indirect_args;

        rdg_buffer_uav pages_to_update;
        rdg_buffer_uav pages_to_update_indirect_args;

        std::uint32_t page_atlas_capacity;

        std::uint32_t distance_field_buffer;
        std::uint32_t brick_table;
        std::uint32_t brick_atlas;

        rdg_buffer_ref invalidated_page_indirect_args;
    };

    graph.add_pass<pass_data>(
        "SDF Mesh Page Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.clipmap_state = pass.add_buffer_uav(m_clipmap_state, RHI_PIPELINE_STAGE_COMPUTE);

            data.clipmap_levels = m_clipmap_levels;
            data.clipmap_level_offset = m_clipmap_level_offset;

            data.invalidated_grids =
                pass.add_buffer_srv(m_invalidated_grids, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidated_pages =
                pass.add_buffer_srv(m_invalidated_pages, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidated_grid_meshes =
                pass.add_buffer_srv(m_invalidated_grid_meshes, RHI_PIPELINE_STAGE_COMPUTE);

            data.mesh_buffer = pass.add_buffer_srv(m_mesh_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            data.page_table = pass.add_texture_uav(
                m_page_table,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_3D);
            data.free_pages = pass.add_buffer_uav(m_free_pages, RHI_PIPELINE_STAGE_COMPUTE);

            data.pages_to_allocate =
                pass.add_buffer_uav(m_pages_to_allocate, RHI_PIPELINE_STAGE_COMPUTE);
            data.pages_to_allocate_indirect_args =
                pass.add_buffer_uav(m_pages_to_allocate_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);

            data.pages_to_update =
                pass.add_buffer_uav(m_pages_to_update, RHI_PIPELINE_STAGE_COMPUTE);
            data.pages_to_update_indirect_args =
                pass.add_buffer_uav(m_pages_to_update_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);

            data.page_atlas_capacity = m_page_atlas_capacity;

            data.distance_field_buffer = m_distance_field_buffer;
            data.brick_table = m_brick_table;
            data.brick_atlas = m_brick_atlas;

            data.invalidated_page_indirect_args = pass.add_buffer(
                m_invalidated_page_indirect_args,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_mesh_page_cull_cs>(),
            });

            command.set_constant(
                sdf_mesh_page_cull_cs::constant_data{
                    .clipmap_state = data.clipmap_state.get_bindless(),
                    .clipmap_levels = data.clipmap_levels,
                    .clipmap_level_offset = data.clipmap_level_offset,
                    .invalidated_grids = data.invalidated_grids.get_bindless(),
                    .invalidated_pages = data.invalidated_pages.get_bindless(),
                    .invalidated_grid_meshes = data.invalidated_grid_meshes.get_bindless(),
                    .mesh_buffer = data.mesh_buffer.get_bindless(),
                    .page_table = data.page_table.get_bindless(),
                    .free_pages = data.free_pages.get_bindless(),
                    .pages_to_allocate = data.pages_to_allocate.get_bindless(),
                    .pages_to_allocate_indirect_args =
                        data.pages_to_allocate_indirect_args.get_bindless(),
                    .pages_to_update = data.pages_to_update.get_bindless(),
                    .pages_to_update_indirect_args =
                        data.pages_to_update_indirect_args.get_bindless(),
                    .page_atlas_capacity = data.page_atlas_capacity,
                    .distance_field_buffer = data.distance_field_buffer,
                    .brick_table = data.brick_table,
                    .brick_atlas = data.brick_atlas,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.invalidated_page_indirect_args.get_rhi());
        });
}

void sdf_pass::allocate_pages(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav clipmap_state;

        rdg_texture_uav page_table;

        rdg_buffer_srv free_pages;

        rdg_buffer_srv pages_to_allocate;
        rdg_buffer_ref pages_to_allocate_indirect_args;

        rdg_buffer_uav pages_to_update;
        rdg_buffer_uav pages_to_update_indirect_args;

        std::uint32_t page_atlas_capacity;
    };

    graph.add_pass<pass_data>(
        "SDF Allocate Pages",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.clipmap_state = pass.add_buffer_uav(m_clipmap_state, RHI_PIPELINE_STAGE_COMPUTE);

            data.page_table = pass.add_texture_uav(
                m_page_table,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_3D);

            data.free_pages = pass.add_buffer_srv(m_free_pages, RHI_PIPELINE_STAGE_COMPUTE);

            data.pages_to_allocate =
                pass.add_buffer_srv(m_pages_to_allocate, RHI_PIPELINE_STAGE_COMPUTE);
            data.pages_to_allocate_indirect_args = pass.add_buffer(
                m_pages_to_allocate_indirect_args,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);

            data.pages_to_update =
                pass.add_buffer_uav(m_pages_to_update, RHI_PIPELINE_STAGE_COMPUTE);
            data.pages_to_update_indirect_args =
                pass.add_buffer_uav(m_pages_to_update_indirect_args, RHI_PIPELINE_STAGE_COMPUTE);

            data.page_atlas_capacity = m_page_atlas_capacity;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_allocate_pages_cs>(),
            });

            command.set_constant(
                sdf_allocate_pages_cs::constant_data{
                    .clipmap_state = data.clipmap_state.get_bindless(),
                    .page_table = data.page_table.get_bindless(),
                    .free_pages = data.free_pages.get_bindless(),
                    .pages_to_allocate = data.pages_to_allocate.get_bindless(),
                    .pages_to_update = data.pages_to_update.get_bindless(),
                    .pages_to_update_indirect_args =
                        data.pages_to_update_indirect_args.get_bindless(),
                    .page_atlas_capacity = data.page_atlas_capacity,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.pages_to_allocate_indirect_args.get_rhi());
        });
}

void sdf_pass::update_pages(render_graph& graph)
{
    struct pass_data
    {
        std::uint32_t clipmap_levels;
        std::uint32_t clipmap_level_offset;

        rdg_buffer_srv invalidated_grids;
        rdg_buffer_srv invalidated_grid_meshes;

        rdg_buffer_srv mesh_buffer;

        rdg_texture_srv page_table;
        rdg_texture_srv page_atlas;

        rdg_buffer_srv pages_to_update;

        std::uint32_t distance_field_buffer;
        std::uint32_t brick_table;
        std::uint32_t brick_atlas;

        rdg_buffer_ref pages_to_update_indirect_args;
    };

    graph.add_pass<pass_data>(
        "SDF Update Pages",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.clipmap_levels = m_clipmap_levels;
            data.clipmap_level_offset = m_clipmap_level_offset;

            data.invalidated_grids =
                pass.add_buffer_srv(m_invalidated_grids, RHI_PIPELINE_STAGE_COMPUTE);
            data.invalidated_grid_meshes =
                pass.add_buffer_srv(m_invalidated_grid_meshes, RHI_PIPELINE_STAGE_COMPUTE);

            data.mesh_buffer = pass.add_buffer_srv(m_mesh_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            data.page_table = pass.add_texture_srv(
                m_page_table,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_3D);
            data.page_atlas = pass.add_texture_srv(
                m_page_atlas,
                RHI_PIPELINE_STAGE_COMPUTE,
                RHI_TEXTURE_DIMENSION_3D);

            data.pages_to_update =
                pass.add_buffer_srv(m_pages_to_update, RHI_PIPELINE_STAGE_COMPUTE);

            data.distance_field_buffer = m_distance_field_buffer;
            data.brick_table = m_brick_table;
            data.brick_atlas = m_brick_atlas;

            data.pages_to_update_indirect_args = pass.add_buffer(
                m_pages_to_update_indirect_args,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<sdf_update_pages_cs>(),
            });

            command.set_constant(
                sdf_update_pages_cs::constant_data{
                    .clipmap_levels = data.clipmap_levels,
                    .clipmap_level_offset = data.clipmap_level_offset,
                    .invalidated_grids = data.invalidated_grids.get_bindless(),
                    .invalidated_grid_meshes = data.invalidated_grid_meshes.get_bindless(),
                    .mesh_buffer = data.mesh_buffer.get_bindless(),
                    .page_table = data.page_table.get_bindless(),
                    .page_atlas = data.page_atlas.get_bindless(),
                    .pages_to_update = data.pages_to_update.get_bindless(),
                    .distance_field_buffer = data.distance_field_buffer,
                    .brick_table = data.brick_table,
                    .brick_atlas = data.brick_atlas,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.pages_to_update_indirect_args.get_rhi());
        });
}

void sdf_pass::add_debug_pass(render_graph& graph, const parameter& parameter)
{
    if (parameter.debug_mode == DEBUG_MODE_PAGE)
    {
        struct pass_data
        {
            rdg_texture_uav debug_output;
            rdg_texture_srv depth_buffer;
        };

        graph.add_pass<pass_data>(
            "SDF Debug",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.debug_output =
                    pass.add_texture_uav(parameter.debug_output, RHI_PIPELINE_STAGE_COMPUTE);
                data.depth_buffer =
                    pass.add_texture_srv(parameter.depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<sdf_debug_page_cs>(),
                });

                command.set_constant(
                    sdf_debug_page_cs::constant_data{
                        .debug_output = data.debug_output.get_bindless(),
                        .depth_buffer = data.depth_buffer.get_bindless(),
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_CAMERA);

                auto extent = data.debug_output.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }
    else if (parameter.debug_mode == DEBUG_MODE_MESH_SDF)
    {
        struct pass_data
        {
            rdg_buffer_srv mesh_buffer;

            std::uint32_t distance_field_buffer;
            std::uint32_t brick_table;
            std::uint32_t brick_atlas;

            rdg_texture_uav debug_output;
        };

        graph.add_pass<pass_data>(
            "SDF Debug",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.mesh_buffer = pass.add_buffer_srv(m_mesh_buffer, RHI_PIPELINE_STAGE_COMPUTE);

                data.distance_field_buffer = m_distance_field_buffer;
                data.brick_table = m_brick_table;
                data.brick_atlas = m_brick_atlas;

                data.debug_output =
                    pass.add_texture_uav(parameter.debug_output, RHI_PIPELINE_STAGE_COMPUTE);
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<sdf_debug_mesh_sdf_cs>(),
                });

                command.set_constant(
                    sdf_debug_mesh_sdf_cs::constant_data{
                        .mesh_index = 0,
                        .mesh_buffer = data.mesh_buffer.get_bindless(),
                        .distance_field_buffer = data.distance_field_buffer,
                        .brick_table = data.brick_table,
                        .brick_atlas = data.brick_atlas,
                        .debug_output = data.debug_output.get_bindless(),
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_CAMERA);

                auto extent = data.debug_output.get_extent();
                command.dispatch_2d(extent.width, extent.height);
            });
    }
}
} // namespace violet
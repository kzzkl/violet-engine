#include "graphics/renderers/passes/shadow_pass.hpp"
#include "graphics/renderers/passes/scan_pass.hpp"
#include "virtual_shadow_map/vsm_common.hpp"

namespace violet
{
struct vsm_prepare_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/prepare.hlsl";

    struct constant_data
    {
        std::uint32_t virtual_page_dispatch_buffer;
        std::uint32_t visible_light_count;
        std::uint32_t lru_state;
        std::uint32_t lru_curr_index;
        std::uint32_t draw_count_buffer;
        std::uint32_t clear_physical_page_dispatch_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_light_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/light_cull.hlsl";

    struct constant_data
    {
        std::uint32_t visible_light_count;
        std::uint32_t visible_light_ids;
        std::uint32_t visible_vsm_ids;
        std::uint32_t virtual_page_dispatch_buffer;
        std::uint32_t camera_id;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

struct vsm_clear_page_table_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/virtual_shadow_map/clear_page_table.hlsl";

    struct constant_data
    {
        std::uint32_t visible_vsm_ids;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_bounds_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_mark_visible_pages_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/virtual_shadow_map/mark_visible_pages.hlsl";

    struct constant_data
    {
        std::uint32_t depth_buffer;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t visible_light_count;
        std::uint32_t visible_light_ids;
        std::uint32_t visible_vsm_ids;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

struct vsm_mark_resident_pages_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/virtual_shadow_map/mark_resident_pages.hlsl";

    struct constant_data
    {
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_physical_page_table;
        std::uint32_t vsm_buffer;
        std::uint32_t frame;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_lru_mark_invalid_pages_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/update_lru.hlsl";
    static constexpr std::string_view entry_point = "mark_invalid_pages";

    struct constant_data
    {
        std::uint32_t vsm_physical_page_table;
        std::uint32_t lru_state;
        std::uint32_t lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t lru_prev_index;
        std::uint32_t lru_remap;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_lru_remove_invalid_pages_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/update_lru.hlsl";
    static constexpr std::string_view entry_point = "remove_invalid_pages";

    struct constant_data
    {
        std::uint32_t vsm_physical_page_table;
        std::uint32_t lru_state;
        std::uint32_t lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t lru_prev_index;
        std::uint32_t lru_remap;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_lru_append_unused_pages_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/update_lru.hlsl";
    static constexpr std::string_view entry_point = "append_unused_pages";

    struct constant_data
    {
        std::uint32_t vsm_physical_page_table;
        std::uint32_t lru_state;
        std::uint32_t lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t lru_prev_index;
        std::uint32_t lru_remap;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_allocate_pages_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/virtual_shadow_map/allocate_pages.hlsl";

    struct constant_data
    {
        std::uint32_t visible_vsm_ids;
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_physical_page_table;
        std::uint32_t lru_state;
        std::uint32_t lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t clear_physical_page_dispatch_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_clear_physical_page_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/virtual_shadow_map/clear_physical_page.hlsl";

    struct constant_data
    {
        std::uint32_t lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t vsm_physical_texture;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_calculate_page_bounds_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/virtual_shadow_map/calculate_page_bounds.hlsl";

    struct constant_data
    {
        std::uint32_t visible_vsm_ids;
        std::uint32_t vsm_bounds_buffer;
        std::uint32_t vsm_virtual_page_table;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_instance_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/instance_cull.hlsl";

    struct constant_data
    {
        std::uint32_t visible_light_count;
        std::uint32_t visible_vsm_ids;
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_bounds_buffer;
        std::uint32_t draw_buffer;
        std::uint32_t draw_count_buffer;
        std::uint32_t draw_info_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

struct vsm_shadow_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/render_shadow.hlsl";

    struct constant_data
    {
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_physical_texture;
        std::uint32_t draw_info_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

struct vsm_shadow_fs : public shader_fs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/render_shadow.hlsl";

    using constant_data = vsm_shadow_vs::constant_data;

    static constexpr parameter_layout parameters = vsm_shadow_vs::parameters;
};

struct vsm_debug_constant_data
{
    std::uint32_t debug_info;
    std::uint32_t debug_info_index;
    std::uint32_t debug_output;
    std::uint32_t depth_buffer;
    std::uint32_t vsm_buffer;
    std::uint32_t vsm_virtual_page_table;
    std::uint32_t vsm_physical_page_table;
    std::uint32_t vsm_bounds_buffer;
    std::uint32_t vsm_physical_texture;
    std::uint32_t draw_count_buffer;
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t light_id;
};

struct vsm_debug_info_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/vsm_debug.hlsl";
    static constexpr std::string_view entry_point = "debug_info";

    using constant_data = vsm_debug_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

struct vsm_debug_page_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/vsm_debug.hlsl";
    static constexpr std::string_view entry_point = "debug_page";

    using constant_data = vsm_debug_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

struct vsm_debug_page_cache_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/vsm_debug.hlsl";
    static constexpr std::string_view entry_point = "debug_page_cache";

    using constant_data = vsm_debug_constant_data;

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

void shadow_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Shadow Pass");

    m_depth_buffer = parameter.depth_buffer;

    m_vsm_buffer = parameter.vsm_buffer;
    m_vsm_virtual_page_table = parameter.vsm_virtual_page_table;
    m_vsm_physical_page_table = parameter.vsm_physical_page_table;
    m_vsm_physical_texture = parameter.vsm_physical_texture;

    m_lru_state = parameter.lru_state;
    m_lru_buffer = parameter.lru_buffer;
    m_lru_curr_index = parameter.lru_curr_index;
    m_lru_prev_index = parameter.lru_prev_index;

    m_vsm_bounds_buffer = graph.add_buffer(
        "VSM Projection Buffer",
        sizeof(vec4u) * MAX_VSM_COUNT,
        RHI_BUFFER_STORAGE);

    m_visible_light_count = graph.add_buffer(
        "VSM Light Cull Result",
        sizeof(std::uint32_t) * 2, // light count, vsm count.
        RHI_BUFFER_STORAGE);
    m_visible_light_ids = graph.add_buffer(
        "VSM Visible Light IDs",
        MAX_SHADOW_LIGHT_COUNT * sizeof(std::uint32_t), // light id.
        RHI_BUFFER_STORAGE);
    m_visible_vsm_ids = graph.add_buffer(
        "VSM Visible VSM IDs",
        MAX_VSM_COUNT * sizeof(std::uint32_t), // vsm id.
        RHI_BUFFER_STORAGE);

    m_virtual_page_dispatch_buffer = graph.add_buffer(
        "VSM Virtual Page Dispatch Buffer",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_clear_physical_page_dispatch_buffer = graph.add_buffer(
        "VSM Clear Physical Page Dispatch Buffer",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    m_draw_buffer = graph.add_buffer(
        "VSM Draw Buffer",
        MAX_SHADOW_DRAWS_PER_FRAME * sizeof(shader::draw_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);
    m_draw_count_buffer = graph.add_buffer(
        "Draw Count Buffer",
        sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);
    m_draw_info_buffer = graph.add_buffer(
        "Draw Info Buffer",
        MAX_SHADOW_DRAWS_PER_FRAME * sizeof(vec2u), // x: vsm id, y: instance id.
        RHI_BUFFER_STORAGE);

    prepare(graph);
    light_cull(graph);
    clear_page_table(graph);
    mark_visible_pages(graph);
    mark_resident_pages(graph);
    update_lru(graph);
    allocate_pages(graph);
    clear_physical_page(graph);
    calculate_page_bounds(graph);
    instance_cull(graph);
    render_shadow(graph);

    m_debug_mode = parameter.debug_mode;
    m_debug_output = parameter.debug_output;
    m_debug_info = parameter.debug_info;
    add_debug_pass(graph);
}

void shadow_pass::prepare(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav virtual_page_dispatch_buffer;
        rdg_buffer_uav visible_light_count;
        rdg_buffer_uav lru_state;
        std::uint32_t lru_curr_index;
        rdg_buffer_uav draw_count_buffer;
        rdg_buffer_uav clear_physical_page_dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Prepare",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.virtual_page_dispatch_buffer =
                pass.add_buffer_uav(m_virtual_page_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_light_count =
                pass.add_buffer_uav(m_visible_light_count, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_state = pass.add_buffer_uav(m_lru_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_curr_index = m_lru_curr_index;
            data.draw_count_buffer =
                pass.add_buffer_uav(m_draw_count_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.clear_physical_page_dispatch_buffer = pass.add_buffer_uav(
                m_clear_physical_page_dispatch_buffer,
                RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_prepare_cs>(),
            });

            command.set_constant(
                vsm_prepare_cs::constant_data{
                    .virtual_page_dispatch_buffer =
                        data.virtual_page_dispatch_buffer.get_bindless(),
                    .visible_light_count = data.visible_light_count.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
                    .draw_count_buffer = data.draw_count_buffer.get_bindless(),
                    .clear_physical_page_dispatch_buffer =
                        data.clear_physical_page_dispatch_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(1, 1);
        });
}

void shadow_pass::light_cull(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav visible_light_count;
        rdg_buffer_uav visible_light_ids;
        rdg_buffer_uav visible_vsm_ids;
        rdg_buffer_uav virtual_page_dispatch_buffer;
        std::uint32_t light_count;
        std::uint32_t camera_id;
    };

    graph.add_pass<pass_data>(
        "VSM Light Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visible_light_count =
                pass.add_buffer_uav(m_visible_light_count, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_light_ids =
                pass.add_buffer_uav(m_visible_light_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_vsm_ids =
                pass.add_buffer_uav(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.virtual_page_dispatch_buffer =
                pass.add_buffer_uav(m_virtual_page_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.light_count = graph.get_scene().get_light_count();
            data.camera_id = graph.get_camera().get_id();
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_light_cull_cs>(),
            });

            command.set_constant(
                vsm_light_cull_cs::constant_data{
                    .visible_light_count = data.visible_light_count.get_bindless(),
                    .visible_light_ids = data.visible_light_ids.get_bindless(),
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .virtual_page_dispatch_buffer =
                        data.virtual_page_dispatch_buffer.get_bindless(),
                    .camera_id = data.camera_id,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.dispatch_1d(data.light_count);
        });
}

void shadow_pass::clear_page_table(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_uav vsm_virtual_page_table;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_uav vsm_bounds_buffer;
        rdg_buffer_ref virtual_page_dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Clear Page Table",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_virtual_page_table =
                pass.add_buffer_uav(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_bounds_buffer =
                pass.add_buffer_uav(m_vsm_bounds_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.virtual_page_dispatch_buffer = pass.add_buffer(
                m_virtual_page_dispatch_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_clear_page_table_cs>(),
            });

            command.set_constant(
                vsm_clear_page_table_cs::constant_data{
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .vsm_bounds_buffer = data.vsm_bounds_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.virtual_page_dispatch_buffer.get_rhi());
        });
}

void shadow_pass::mark_visible_pages(render_graph& graph)
{
    struct pass_data
    {
        rdg_texture_srv depth_buffer;
        rdg_buffer_srv visible_light_count;
        rdg_buffer_srv visible_light_ids;
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_uav vsm_virtual_page_table;
        rdg_buffer_srv vsm_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Mark Visible Pages",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.depth_buffer = pass.add_texture_srv(m_depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_light_count =
                pass.add_buffer_srv(m_visible_light_count, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_light_ids =
                pass.add_buffer_srv(m_visible_light_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_virtual_page_table =
                pass.add_buffer_uav(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            auto extent = data.depth_buffer.get_extent();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_mark_visible_pages_cs>(),
            });
            command.set_constant(
                vsm_mark_visible_pages_cs::constant_data{
                    .depth_buffer = data.depth_buffer.get_bindless(),
                    .width = extent.width,
                    .height = extent.height,
                    .visible_light_count = data.visible_light_count.get_bindless(),
                    .visible_light_ids = data.visible_light_ids.get_bindless(),
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.dispatch_2d(extent.width, extent.height);
        });
}

void shadow_pass::mark_resident_pages(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav vsm_virtual_page_table;
        rdg_buffer_uav vsm_physical_page_table;
        rdg_buffer_srv vsm_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Mark Resident Pages",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.vsm_virtual_page_table =
                pass.add_buffer_uav(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_physical_page_table =
                pass.add_buffer_uav(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_mark_resident_pages_cs>(),
            });

            command.set_constant(
                vsm_mark_resident_pages_cs::constant_data{
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .frame = device.get_frame_count(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT);
        });
}

void shadow_pass::update_lru(render_graph& graph)
{
    rdg_scope scope(graph, "VSM Update LRU");

    rdg_buffer* lru_remap = graph.add_buffer(
        "VSM LRU Remap",
        sizeof(std::uint32_t) * VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT,
        RHI_BUFFER_STORAGE);

    // Mark invalid pages.
    struct mark_invalid_pages_pass_data
    {
        rdg_buffer_uav vsm_physical_page_table;
        rdg_buffer_srv lru_state;
        rdg_buffer_uav lru_buffer;
        std::uint32_t lru_prev_index;
        rdg_buffer_uav lru_remap;
    };

    graph.add_pass<mark_invalid_pages_pass_data>(
        "Mark Invalid Pages",
        RDG_PASS_COMPUTE,
        [&](mark_invalid_pages_pass_data& data, rdg_pass& pass)
        {
            data.vsm_physical_page_table =
                pass.add_buffer_uav(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_state = pass.add_buffer_srv(m_lru_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_buffer = pass.add_buffer_uav(m_lru_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_remap = pass.add_buffer_uav(lru_remap, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_prev_index = m_lru_prev_index;
        },
        [](const mark_invalid_pages_pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_lru_mark_invalid_pages_cs>(),
            });

            command.set_constant(
                vsm_lru_mark_invalid_pages_cs::constant_data{
                    .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_prev_index = data.lru_prev_index,
                    .lru_remap = data.lru_remap.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT);
        });

    // Scan invalid pages.
    graph.add_pass<scan_pass>({
        .buffer = lru_remap,
        .size = VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT,
    });

    // Remove invalid pages.
    struct remove_invalid_pages_pass_data
    {
        rdg_buffer_uav lru_state;
        rdg_buffer_uav lru_buffer;
        std::uint32_t lru_curr_index;
        std::uint32_t lru_prev_index;
        rdg_buffer_srv lru_remap;
    };
    graph.add_pass<remove_invalid_pages_pass_data>(
        "Remove Invalid Pages",
        RDG_PASS_COMPUTE,
        [&](remove_invalid_pages_pass_data& data, rdg_pass& pass)
        {
            data.lru_state = pass.add_buffer_uav(m_lru_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_buffer = pass.add_buffer_uav(m_lru_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_curr_index = m_lru_curr_index;
            data.lru_prev_index = m_lru_prev_index;
            data.lru_remap = pass.add_buffer_srv(lru_remap, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const remove_invalid_pages_pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_lru_remove_invalid_pages_cs>(),
            });

            command.set_constant(
                vsm_lru_remove_invalid_pages_cs::constant_data{
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
                    .lru_prev_index = data.lru_prev_index,
                    .lru_remap = data.lru_remap.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT);
        });

    // Append unused pages.
    struct append_unused_pages_pass_data
    {
        rdg_buffer_uav vsm_physical_page_table;
        rdg_buffer_uav lru_state;
        rdg_buffer_uav lru_buffer;
        std::uint32_t lru_curr_index;
    };

    graph.add_pass<append_unused_pages_pass_data>(
        "Append Unused Pages",
        RDG_PASS_COMPUTE,
        [&](append_unused_pages_pass_data& data, rdg_pass& pass)
        {
            data.vsm_physical_page_table =
                pass.add_buffer_uav(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_state = pass.add_buffer_uav(m_lru_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_buffer = pass.add_buffer_uav(m_lru_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_curr_index = m_lru_curr_index;
        },
        [](const append_unused_pages_pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_lru_append_unused_pages_cs>(),
            });

            command.set_constant(
                vsm_lru_append_unused_pages_cs::constant_data{
                    .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT);
        });
}

void shadow_pass::allocate_pages(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_uav vsm_virtual_page_table;
        rdg_buffer_uav vsm_physical_page_table;
        rdg_buffer_uav lru_state;
        rdg_buffer_srv lru_buffer;
        std::uint32_t lru_curr_index;
        rdg_buffer_uav clear_physical_page_dispatch_buffer;
        rdg_buffer_ref virtual_page_dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Allocate Pages",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_virtual_page_table =
                pass.add_buffer_uav(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_physical_page_table =
                pass.add_buffer_uav(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_state = pass.add_buffer_uav(m_lru_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_buffer = pass.add_buffer_srv(m_lru_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_curr_index = m_lru_curr_index;

            data.clear_physical_page_dispatch_buffer = pass.add_buffer_uav(
                m_clear_physical_page_dispatch_buffer,
                RHI_PIPELINE_STAGE_COMPUTE);

            data.virtual_page_dispatch_buffer = pass.add_buffer(
                m_virtual_page_dispatch_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_allocate_pages_cs>(),
            });

            command.set_constant(
                vsm_allocate_pages_cs::constant_data{
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
                    .clear_physical_page_dispatch_buffer =
                        data.clear_physical_page_dispatch_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.virtual_page_dispatch_buffer.get_rhi());
        });
}

void shadow_pass::clear_physical_page(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv lru_buffer;
        std::uint32_t lru_curr_index;
        rdg_texture_uav vsm_physical_texture;
        rdg_buffer_ref clear_physical_page_dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Clear Physical Page",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.lru_buffer = pass.add_buffer_srv(m_lru_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_curr_index = m_lru_curr_index;
            data.vsm_physical_texture =
                pass.add_texture_uav(m_vsm_physical_texture, RHI_PIPELINE_STAGE_COMPUTE);
            data.clear_physical_page_dispatch_buffer = pass.add_buffer(
                m_clear_physical_page_dispatch_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_clear_physical_page_cs>(),
            });

            command.set_constant(
                vsm_clear_physical_page_cs::constant_data{
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
                    .vsm_physical_texture = data.vsm_physical_texture.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.clear_physical_page_dispatch_buffer.get_rhi());
        });
}

void shadow_pass::calculate_page_bounds(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_uav vsm_bounds_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_buffer_ref virtual_page_dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Calculate Page Bounds",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_bounds_buffer =
                pass.add_buffer_uav(m_vsm_bounds_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_virtual_page_table =
                pass.add_buffer_srv(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.virtual_page_dispatch_buffer = pass.add_buffer(
                m_virtual_page_dispatch_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_calculate_page_bounds_cs>(),
            });

            command.set_constant(
                vsm_calculate_page_bounds_cs::constant_data{
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .vsm_bounds_buffer = data.vsm_bounds_buffer.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.virtual_page_dispatch_buffer.get_rhi());
        });
}

void shadow_pass::instance_cull(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv visible_light_count;
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_buffer_srv vsm_bounds_buffer;
        rdg_buffer_uav draw_buffer;
        rdg_buffer_uav draw_count_buffer;
        rdg_buffer_uav draw_info_buffer;

        std::uint32_t instance_count;
    };

    graph.add_pass<pass_data>(
        "VSM Instance Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visible_light_count =
                pass.add_buffer_srv(m_visible_light_count, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_virtual_page_table =
                pass.add_buffer_srv(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_bounds_buffer =
                pass.add_buffer_srv(m_vsm_bounds_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.draw_buffer = pass.add_buffer_uav(m_draw_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.draw_count_buffer =
                pass.add_buffer_uav(m_draw_count_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.draw_info_buffer =
                pass.add_buffer_uav(m_draw_info_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            data.instance_count = graph.get_scene().get_instance_count();
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_instance_cull_cs>(),
            });

            command.set_constant(
                vsm_instance_cull_cs::constant_data{
                    .visible_light_count = data.visible_light_count.get_bindless(),
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_bounds_buffer = data.vsm_bounds_buffer.get_bindless(),
                    .draw_buffer = data.draw_buffer.get_bindless(),
                    .draw_count_buffer = data.draw_count_buffer.get_bindless(),
                    .draw_info_buffer = data.draw_info_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);

            command.dispatch_1d(data.instance_count);
        });
}

void shadow_pass::render_shadow(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_texture_uav vsm_physical_texture;
        rdg_buffer_ref draw_buffer;
        rdg_buffer_ref draw_count_buffer;
        rdg_buffer_srv draw_info_buffer;

        std::uint32_t instance_count;
    };

    graph.add_pass<pass_data>(
        "VSM Render Shadow",
        RDG_PASS_RASTER,
        [&](pass_data& data, rdg_pass& pass)
        {
            pass.set_render_area({
                .width = VSM_VIRTUAL_RESOLUTION,
                .height = VSM_VIRTUAL_RESOLUTION,
            });

            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_VERTEX);
            data.vsm_virtual_page_table =
                pass.add_buffer_srv(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_FRAGMENT);
            data.vsm_physical_texture =
                pass.add_texture_uav(m_vsm_physical_texture, RHI_PIPELINE_STAGE_FRAGMENT);
            data.draw_buffer = pass.add_buffer(
                m_draw_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.draw_count_buffer = pass.add_buffer(
                m_draw_count_buffer,
                RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                RHI_ACCESS_INDIRECT_COMMAND_READ);
            data.draw_info_buffer =
                pass.add_buffer_srv(m_draw_info_buffer, RHI_PIPELINE_STAGE_VERTEX);

            data.instance_count = graph.get_scene().get_instance_count();
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .vertex_shader = device.get_shader<vsm_shadow_vs>(),
                .fragment_shader = device.get_shader<vsm_shadow_fs>(),
                .rasterizer_state = device.get_rasterizer_state<RHI_CULL_MODE_FRONT>(),
                .depth_stencil_state =
                    device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
            });

            command.set_constant(
                vsm_shadow_vs::constant_data{
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_physical_texture = data.vsm_physical_texture.get_bindless(),
                    .draw_info_buffer = data.draw_info_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_index_buffer();

            command.set_viewport({
                .width = VSM_VIRTUAL_RESOLUTION,
                .height = VSM_VIRTUAL_RESOLUTION,
                .min_depth = 0.0f,
                .max_depth = 1.0f,
            });

            rhi_scissor_rect scissor = {
                .max_x = VSM_VIRTUAL_RESOLUTION,
                .max_y = VSM_VIRTUAL_RESOLUTION,
            };
            command.set_scissor({&scissor, 1});

            command.draw_indexed_indirect(
                data.draw_buffer.get_rhi(),
                0,
                data.draw_count_buffer.get_rhi(),
                0,
                MAX_SHADOW_DRAWS_PER_FRAME);
        });
}

void shadow_pass::add_debug_pass(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav debug_info;
        rdg_texture_uav debug_output;
        rdg_texture_srv depth_buffer;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_buffer_srv vsm_physical_page_table;
        rdg_buffer_srv vsm_bounds_buffer;
        rdg_texture_srv vsm_physical_texture;
        rdg_buffer_srv draw_count_buffer;
        std::uint32_t light_id;
    };

    if (m_debug_info != nullptr)
    {
        graph.add_pass<pass_data>(
            "VSM Debug Info",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.debug_info = pass.add_buffer_uav(m_debug_info, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_virtual_page_table =
                    pass.add_buffer_srv(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
                data.draw_count_buffer =
                    pass.add_buffer_srv(m_draw_count_buffer, RHI_PIPELINE_STAGE_COMPUTE);

                data.light_id = m_debug_light_id;
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<vsm_debug_info_cs>(),
                });

                command.set_constant(
                    vsm_debug_page_cs::constant_data{
                        .debug_info = data.debug_info.get_bindless(),
                        .debug_info_index = device.get_frame_resource_index(),
                        .vsm_buffer = data.vsm_buffer.get_bindless(),
                        .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                        .draw_count_buffer = data.draw_count_buffer.get_bindless(),
                        .light_id = data.light_id,
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_SCENE);
                command.set_parameter(2, RDG_PARAMETER_CAMERA);

                command.dispatch_2d(VSM_VIRTUAL_PAGE_TABLE_SIZE, VSM_VIRTUAL_PAGE_TABLE_SIZE);
            });
    }

    if (m_debug_mode == DEBUG_MODE_PAGE)
    {
        graph.add_pass<pass_data>(
            "VSM Debug Page",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.debug_output =
                    pass.add_texture_uav(m_debug_output, RHI_PIPELINE_STAGE_COMPUTE);
                data.depth_buffer =
                    pass.add_texture_srv(m_depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.light_id = m_debug_light_id;
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<vsm_debug_page_cs>(),
                });

                rhi_texture_extent extent = data.debug_output.get_extent();

                command.set_constant(
                    vsm_debug_page_cs::constant_data{
                        .debug_output = data.debug_output.get_bindless(),
                        .depth_buffer = data.depth_buffer.get_bindless(),
                        .vsm_buffer = data.vsm_buffer.get_bindless(),
                        .width = extent.width,
                        .height = extent.height,
                        .light_id = data.light_id,
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_SCENE);
                command.set_parameter(2, RDG_PARAMETER_CAMERA);

                command.dispatch_2d(extent.width, extent.height);
            });
    }
    else if (m_debug_mode == DEBUG_MODE_PAGE_CACHE)
    {
        graph.add_pass<pass_data>(
            "VSM Debug Page Cache",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.debug_output =
                    pass.add_texture_uav(m_debug_output, RHI_PIPELINE_STAGE_COMPUTE);
                data.depth_buffer =
                    pass.add_texture_srv(m_depth_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_virtual_page_table =
                    pass.add_buffer_srv(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
                data.light_id = m_debug_light_id;
            },
            [](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                command.set_pipeline({
                    .compute_shader = device.get_shader<vsm_debug_page_cache_cs>(),
                });

                rhi_texture_extent extent = data.debug_output.get_extent();

                command.set_constant(
                    vsm_debug_page_cache_cs::constant_data{
                        .debug_output = data.debug_output.get_bindless(),
                        .depth_buffer = data.depth_buffer.get_bindless(),
                        .vsm_buffer = data.vsm_buffer.get_bindless(),
                        .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                        .width = extent.width,
                        .height = extent.height,
                        .light_id = data.light_id,
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_SCENE);
                command.set_parameter(2, RDG_PARAMETER_CAMERA);

                command.dispatch_2d(extent.width, extent.height);
            });
    }

    // graph.add_pass<pass_data>(
    //     "VSM Debug",
    //     RDG_PASS_COMPUTE,
    //     [&](pass_data& data, rdg_pass& pass)
    //     {
    //         data.visible_vsm_ids =
    //             pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
    //         data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
    //         data.vsm_virtual_page_table =
    //             pass.add_buffer_srv(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
    //         data.vsm_physical_page_table =
    //             pass.add_buffer_srv(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
    //         data.vsm_bounds_buffer =
    //             pass.add_buffer_srv(m_vsm_bounds_buffer, RHI_PIPELINE_STAGE_COMPUTE);
    //         data.vsm_physical_texture =
    //             pass.add_texture_srv(m_vsm_physical_texture, RHI_PIPELINE_STAGE_COMPUTE);
    //         data.debug_output = pass.add_texture_uav(m_debug_output, RHI_PIPELINE_STAGE_COMPUTE);
    //     },
    //     [](const pass_data& data, rdg_command& command)
    //     {
    //         auto& device = render_device::instance();

    //         command.set_pipeline({
    //             .compute_shader = device.get_shader<vsm_debug_cs>(),
    //         });

    //         command.set_constant(
    //             vsm_debug_cs::constant_data{
    //                 .debug_output = data.debug_output.get_bindless(),
    //                 .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
    //                 .vsm_buffer = data.vsm_buffer.get_bindless(),
    //                 .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
    //                 .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
    //                 .vsm_bounds_buffer = data.vsm_bounds_buffer.get_bindless(),
    //                 .vsm_physical_texture = data.vsm_physical_texture.get_bindless(),
    //             });

    //         command.set_parameter(0, RDG_PARAMETER_BINDLESS);
    //         command.set_parameter(1, RDG_PARAMETER_SCENE);

    //         command.dispatch_2d(256, 512 + 256);
    //     });
}
} // namespace violet
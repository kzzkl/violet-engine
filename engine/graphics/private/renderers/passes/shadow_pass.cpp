#include "graphics/renderers/passes/shadow_pass.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/renderers/passes/scan_pass.hpp"
#include "virtual_shadow_map/vsm_common.hpp"
#include <format>

namespace violet
{
static constexpr std::uint32_t MAX_SHADOW_DRAWS_PER_FRAME = 1024 * 100;
static constexpr std::uint32_t STATIC_INSTANCE_DRAW_OFFSET = 0;
static constexpr std::uint32_t DYNAMIC_INSTANCE_DRAW_OFFSET =
    MAX_SHADOW_DRAWS_PER_FRAME * sizeof(shader::draw_command);

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
        std::uint32_t cluster_queue_state;
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
        std::uint32_t vsm_directional_buffer;
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
        std::uint32_t vsm_directional_buffer;
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
        std::uint32_t vsm_physical_page_table;
        std::uint32_t vsm_physical_shadow_map_static;
        std::uint32_t vsm_physical_shadow_map_final;
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
        std::uint32_t vsm_physical_page_table;
        std::uint32_t vsm_bounds_buffer;
        std::uint32_t hzb;
        std::uint32_t hzb_sampler;
        std::uint32_t draw_buffer;
        std::uint32_t draw_count_buffer;
        std::uint32_t draw_info_buffer;
        std::uint32_t cluster_queue;
        std::uint32_t cluster_queue_state;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

struct vsm_prepare_cluster_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/prepare_cluster_cull.hlsl";

    struct constant_data
    {
        std::uint32_t cluster_queue_state;
        std::uint32_t dispatch_buffer;
        std::uint32_t recheck;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_cluster_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/cluster_cull.hlsl";

    struct constant_data
    {
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_bounds_buffer;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_physical_page_table;

        std::uint32_t hzb;
        std::uint32_t hzb_sampler;

        float threshold;

        std::uint32_t cluster_node_buffer;
        std::uint32_t cluster_queue;
        std::uint32_t cluster_queue_state;

        std::uint32_t draw_buffer;
        std::uint32_t draw_count_buffer;
        std::uint32_t draw_info_buffer;

        std::uint32_t max_cluster_count;
        std::uint32_t max_cluster_node_count;
        std::uint32_t max_draw_command_count;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
    };
};

struct vsm_build_hzb_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/build_hzb.hlsl";

    struct constant_data
    {
        std::uint32_t vsm_physical_page_table;
        std::uint32_t prev_buffer;
        std::uint32_t next_buffer;
        std::uint32_t hzb_sampler;
        std::uint32_t next_size;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_shadow_vs : public shader_vs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/render_shadow.hlsl";

    struct constant_data
    {
        std::uint32_t vsm_buffer;
        std::uint32_t vsm_virtual_page_table;
        std::uint32_t vsm_physical_shadow_map;
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

struct vsm_merge_physical_page_cs : public shader_cs
{
    static constexpr std::string_view path =
        "assets/shaders/virtual_shadow_map/merge_physical_page.hlsl";

    struct constant_data
    {
        std::uint32_t vsm_physical_page_table;
        std::uint32_t vsm_physical_shadow_map_static;
        std::uint32_t vsm_physical_shadow_map_final;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_debug_constant_data
{
    std::uint32_t debug_info;
    std::uint32_t debug_info_index;
    std::uint32_t debug_output;
    std::uint32_t depth_buffer;
    std::uint32_t vsm_buffer;
    std::uint32_t vsm_virtual_page_table;
    std::uint32_t vsm_directional_buffer;
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
    m_vsm_physical_shadow_map_static = parameter.vsm_physical_shadow_map_static;
    m_vsm_physical_shadow_map_final = parameter.vsm_physical_shadow_map_final;
    m_vsm_directional_buffer = parameter.vsm_directional_buffer;

    if (parameter.vsm_hzb != nullptr)
    {
        m_vsm_hzb = parameter.vsm_hzb;
        m_hzb_sampler = render_device::instance().get_sampler({
            .mag_filter = RHI_FILTER_LINEAR,
            .min_filter = RHI_FILTER_LINEAR,
            .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .min_level = 0.0f,
            .max_level = -1.0f,
            .reduction_mode = RHI_SAMPLER_REDUCTION_MODE_MIN,
        });
    }

    m_lru_state = parameter.lru_state;
    m_lru_buffer = parameter.lru_buffer;
    m_lru_curr_index = parameter.lru_curr_index;
    m_lru_prev_index = parameter.lru_prev_index;

    m_vsm_bounds_buffer = graph.add_buffer(
        "VSM Projection Buffer",
        sizeof(vec4u) * MAX_VSM_COUNT * 2,
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

    // Draw buffer for static and dynamic instances.
    m_draw_buffer = graph.add_buffer(
        "VSM Draw Buffer",
        MAX_SHADOW_DRAWS_PER_FRAME * sizeof(shader::draw_command) * 2,
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);
    m_draw_count_buffer = graph.add_buffer(
        "VSM Draw Count Buffer",
        sizeof(std::uint32_t) * 2,
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);
    m_draw_info_buffer = graph.add_buffer(
        "VSM Draw Info Buffer",
        MAX_SHADOW_DRAWS_PER_FRAME * sizeof(vec2u) * 2, // x: vsm id, y: instance id.
        RHI_BUFFER_STORAGE);

    m_cluster_queue = graph.add_buffer(
        "VSM Cluster Queue",
        graphics_config::get_max_candidate_cluster_count() * sizeof(vec3u),
        RHI_BUFFER_STORAGE);
    m_cluster_queue_state =
        graph.add_buffer("VSM Cluster Queue State", 6 * sizeof(std::uint32_t), RHI_BUFFER_STORAGE);

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
    cluster_cull(graph);
    render_shadow(graph, true);
    render_shadow(graph, false);
    merge_physical_page(graph);

    // build_hzb(graph);

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
        rdg_buffer_uav cluster_queue_state;
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
            data.cluster_queue_state =
                pass.add_buffer_uav(m_cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
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
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
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
        rdg_buffer_srv vsm_directional_buffer;
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
            data.light_count = graph.get_scene().get_light_count(true);
            data.camera_id = graph.get_camera().get_id();
            data.vsm_directional_buffer =
                pass.add_buffer_srv(m_vsm_directional_buffer, RHI_PIPELINE_STAGE_COMPUTE);
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
                    .vsm_directional_buffer = data.vsm_directional_buffer.get_bindless(),
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
        rdg_buffer_srv vsm_directional_buffer;
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
            data.vsm_directional_buffer =
                pass.add_buffer_srv(m_vsm_directional_buffer, RHI_PIPELINE_STAGE_COMPUTE);
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
                    .vsm_directional_buffer = data.vsm_directional_buffer.get_bindless(),
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
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.virtual_page_dispatch_buffer.get_rhi());
        });
}

void shadow_pass::clear_physical_page(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv vsm_physical_page_table;
        rdg_texture_uav vsm_physical_shadow_map_static;
        rdg_texture_uav vsm_physical_shadow_map_final;
    };

    graph.add_pass<pass_data>(
        "VSM Clear Physical Page",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.vsm_physical_page_table =
                pass.add_buffer_srv(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_physical_shadow_map_static =
                pass.add_texture_uav(m_vsm_physical_shadow_map_static, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_physical_shadow_map_final =
                pass.add_texture_uav(m_vsm_physical_shadow_map_final, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_clear_physical_page_cs>(),
            });

            command.set_constant(
                vsm_clear_physical_page_cs::constant_data{
                    .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
                    .vsm_physical_shadow_map_static =
                        data.vsm_physical_shadow_map_static.get_bindless(),
                    .vsm_physical_shadow_map_final =
                        data.vsm_physical_shadow_map_final.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_3d(
                VSM_PAGE_RESOLUTION,
                VSM_PAGE_RESOLUTION,
                VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT,
                8,
                8,
                1);
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
        rdg_buffer_srv vsm_physical_page_table;
        rdg_buffer_srv vsm_bounds_buffer;
        rdg_texture_srv hzb;
        rdg_buffer_uav draw_buffer;
        rdg_buffer_uav draw_count_buffer;
        rdg_buffer_uav draw_info_buffer;
        rdg_buffer_uav cluster_queue;
        rdg_buffer_uav cluster_queue_state;

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
            data.cluster_queue = pass.add_buffer_uav(m_cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
            data.cluster_queue_state =
                pass.add_buffer_uav(m_cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);

            if (m_vsm_hzb != nullptr)
            {
                data.vsm_physical_page_table =
                    pass.add_buffer_srv(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
                data.hzb = pass.add_texture_srv(m_vsm_hzb, RHI_PIPELINE_STAGE_COMPUTE);
            }

            data.instance_count = graph.get_scene().get_instance_count();
        },
        [hzb_sampler = m_hzb_sampler](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            bool use_occlusion = data.hzb;

            std::vector<std::wstring> defines;
            if (use_occlusion)
            {
                defines.emplace_back(L"USE_OCCLUSION");
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_instance_cull_cs>(defines),
            });

            vsm_instance_cull_cs::constant_data constant = {
                .visible_light_count = data.visible_light_count.get_bindless(),
                .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                .vsm_buffer = data.vsm_buffer.get_bindless(),
                .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                .vsm_bounds_buffer = data.vsm_bounds_buffer.get_bindless(),
                .draw_buffer = data.draw_buffer.get_bindless(),
                .draw_count_buffer = data.draw_count_buffer.get_bindless(),
                .draw_info_buffer = data.draw_info_buffer.get_bindless(),
                .cluster_queue = data.cluster_queue.get_bindless(),
                .cluster_queue_state = data.cluster_queue_state.get_bindless(),
            };

            if (use_occlusion)
            {
                constant.vsm_physical_page_table = data.vsm_physical_page_table.get_bindless();
                constant.hzb = data.hzb.get_bindless();
                constant.hzb_sampler = hzb_sampler->get_bindless();
            }

            command.set_constant(constant);

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);

            command.dispatch_1d(data.instance_count);
        });
}

void shadow_pass::prepare_cluster_cull(
    render_graph& graph,
    rdg_buffer* dispatch_buffer,
    bool cull_cluster)
{
    struct pass_data
    {
        rdg_buffer_uav cluster_queue_state;
        rdg_buffer_uav dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "Prepare Cluster Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.cluster_queue_state =
                pass.add_buffer_uav(m_cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.dispatch_buffer = pass.add_buffer_uav(dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [cull_cluster](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines = {
                cull_cluster ? L"-DCULL_CLUSTER" : L"-DCULL_CLUSTER_NODE",
            };

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_prepare_cluster_cull_cs>(defines),
            });

            command.set_constant(
                vsm_prepare_cluster_cull_cs::constant_data{
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
                    .dispatch_buffer = data.dispatch_buffer.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(1, 1);
        });
}

void shadow_pass::cluster_cull(render_graph& graph)
{
    rdg_scope scope(graph, "VSM Cluster Cull");

    rdg_buffer* dispatch_buffer = graph.add_buffer(
        "Dispatch Buffer",
        3 * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    struct pass_data
    {
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv vsm_bounds_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_buffer_srv vsm_physical_page_table;

        rdg_texture_srv hzb;

        rdg_buffer_uav cluster_queue;
        rdg_buffer_uav cluster_queue_state;

        rdg_buffer_uav draw_buffer;
        rdg_buffer_uav draw_count_buffer;
        rdg_buffer_uav draw_info_buffer;

        rdg_buffer_ref dispatch_buffer;
    };

    std::uint32_t cluster_node_depth =
        render_device::instance().get_geometry_manager()->get_cluster_node_depth();
    for (std::uint32_t i = 0; i < cluster_node_depth; ++i)
    {
        bool cull_cluster = i == cluster_node_depth - 1;

        prepare_cluster_cull(graph, dispatch_buffer, cull_cluster);

        graph.add_pass<pass_data>(
            cull_cluster ? "Cull Cluster" : "Cull Cluster Node: " + std::to_string(i),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_bounds_buffer =
                    pass.add_buffer_srv(m_vsm_bounds_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                data.vsm_virtual_page_table =
                    pass.add_buffer_srv(m_vsm_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);

                data.cluster_queue =
                    pass.add_buffer_uav(m_cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
                data.cluster_queue_state =
                    pass.add_buffer_uav(m_cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
                data.dispatch_buffer = pass.add_buffer(
                    dispatch_buffer,
                    RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                    RHI_ACCESS_INDIRECT_COMMAND_READ);

                if (cull_cluster)
                {
                    data.draw_buffer =
                        pass.add_buffer_uav(m_draw_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                    data.draw_count_buffer =
                        pass.add_buffer_uav(m_draw_count_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                    data.draw_info_buffer =
                        pass.add_buffer_uav(m_draw_info_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                }

                if (m_vsm_hzb != nullptr)
                {
                    data.vsm_physical_page_table =
                        pass.add_buffer_srv(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
                    data.hzb = pass.add_texture_srv(m_vsm_hzb, RHI_PIPELINE_STAGE_COMPUTE);
                }
            },
            [cull_cluster, hzb_sampler = m_hzb_sampler](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                std::vector<std::wstring> defines = {
                    cull_cluster ? L"-DCULL_CLUSTER" : L"-DCULL_CLUSTER_NODE",
                };

                bool use_occlusion = data.hzb;
                if (use_occlusion)
                {
                    defines.emplace_back(L"USE_OCCLUSION");
                }

                command.set_pipeline({
                    .compute_shader = device.get_shader<vsm_cluster_cull_cs>(defines),
                });

                vsm_cluster_cull_cs::constant_data constant = {
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .vsm_bounds_buffer = data.vsm_bounds_buffer.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .threshold = 1.0f,
                    .cluster_queue = data.cluster_queue.get_bindless(),
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
                    .max_cluster_count = graphics_config::get_max_cluster_count(),
                    .max_cluster_node_count = graphics_config::get_max_cluster_node_count(),
                    .max_draw_command_count = graphics_config::get_max_draw_command_count(),
                };

                if (cull_cluster)
                {
                    constant.draw_buffer = data.draw_buffer.get_bindless();
                    constant.draw_count_buffer = data.draw_count_buffer.get_bindless();
                    constant.draw_info_buffer = data.draw_info_buffer.get_bindless();
                }
                else
                {
                    rhi_buffer_srv* cluster_node_buffer_srv =
                        device.get_geometry_manager()->get_cluster_node_buffer()->get_srv();
                    constant.cluster_node_buffer = cluster_node_buffer_srv->get_bindless();
                }

                if (use_occlusion)
                {
                    constant.vsm_physical_page_table = data.vsm_physical_page_table.get_bindless();
                    constant.hzb = data.hzb.get_bindless();
                    constant.hzb_sampler = hzb_sampler->get_bindless();
                }

                command.set_constant(constant);
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_SCENE);

                command.dispatch_indirect(data.dispatch_buffer.get_rhi(), 0);
            });
    }
}

void shadow_pass::render_shadow(render_graph& graph, bool is_static)
{
    struct pass_data
    {
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv vsm_virtual_page_table;
        rdg_texture_uav vsm_physical_shadow_map;
        rdg_buffer_ref draw_buffer;
        rdg_buffer_ref draw_count_buffer;
        rdg_buffer_srv draw_info_buffer;

        bool is_static;
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
            data.vsm_physical_shadow_map = pass.add_texture_uav(
                is_static ? m_vsm_physical_shadow_map_static : m_vsm_physical_shadow_map_final,
                RHI_PIPELINE_STAGE_FRAGMENT);
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

            data.is_static = is_static;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .vertex_shader = device.get_shader<vsm_shadow_vs>(),
                .fragment_shader = device.get_shader<vsm_shadow_fs>(),
                .rasterizer_state = device.get_rasterizer_state<RHI_CULL_MODE_NONE>(),
                .depth_stencil_state =
                    device.get_depth_stencil_state<true, true, RHI_COMPARE_OP_GREATER>(),
            });

            command.set_constant(
                vsm_shadow_vs::constant_data{
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .vsm_virtual_page_table = data.vsm_virtual_page_table.get_bindless(),
                    .vsm_physical_shadow_map = data.vsm_physical_shadow_map.get_bindless(),
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
                data.is_static ? STATIC_INSTANCE_DRAW_OFFSET : DYNAMIC_INSTANCE_DRAW_OFFSET,
                data.draw_count_buffer.get_rhi(),
                data.is_static ? 0 : sizeof(std::uint32_t),
                MAX_SHADOW_DRAWS_PER_FRAME);
        });
}

void shadow_pass::merge_physical_page(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv vsm_physical_page_table;
        rdg_texture_srv vsm_physical_shadow_map_static;
        rdg_texture_uav vsm_physical_shadow_map_final;
    };

    graph.add_pass<pass_data>(
        "VSM Merge Physical Page",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.vsm_physical_page_table =
                pass.add_buffer_srv(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_physical_shadow_map_static =
                pass.add_texture_srv(m_vsm_physical_shadow_map_static, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_physical_shadow_map_final =
                pass.add_texture_uav(m_vsm_physical_shadow_map_final, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_merge_physical_page_cs>(),
            });

            command.set_constant(
                vsm_merge_physical_page_cs::constant_data{
                    .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
                    .vsm_physical_shadow_map_static =
                        data.vsm_physical_shadow_map_static.get_bindless(),
                    .vsm_physical_shadow_map_final =
                        data.vsm_physical_shadow_map_final.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_3d(
                VSM_PAGE_RESOLUTION,
                VSM_PAGE_RESOLUTION,
                VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT,
                8,
                8,
                1);
        });
}

void shadow_pass::build_hzb(render_graph& graph)
{
    if (m_vsm_hzb == nullptr)
    {
        return;
    }

    rdg_scope scope(graph, "VSM Build HZB");

    struct pass_data
    {
        rdg_buffer_uav vsm_physical_page_table;
        rdg_texture_srv prev_buffer;
        rdg_texture_uav next_buffer;
    };

    for (std::uint32_t level = 0; level < m_vsm_hzb->get_level_count(); ++level)
    {
        graph.add_pass<pass_data>(
            std::format("Level {}", level),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.vsm_physical_page_table =
                    pass.add_buffer_uav(m_vsm_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);

                if (level == 0)
                {
                    data.prev_buffer = pass.add_texture_srv(
                        m_vsm_physical_shadow_map_static,
                        RHI_PIPELINE_STAGE_COMPUTE);
                }
                else
                {
                    data.prev_buffer = pass.add_texture_srv(
                        m_vsm_hzb,
                        RHI_PIPELINE_STAGE_COMPUTE,
                        RHI_TEXTURE_DIMENSION_2D,
                        level - 1,
                        1);
                }

                data.next_buffer = pass.add_texture_uav(
                    m_vsm_hzb,
                    RHI_PIPELINE_STAGE_COMPUTE,
                    RHI_TEXTURE_DIMENSION_2D,
                    level,
                    1);
            },
            [hzb_sampler = m_hzb_sampler](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                std::uint32_t next_size =
                    data.next_buffer.get_extent().width / VSM_PHYSICAL_PAGE_TABLE_SIZE_X;

                command.set_pipeline({
                    .compute_shader = device.get_shader<vsm_build_hzb_cs>(),
                });

                command.set_constant(
                    vsm_build_hzb_cs::constant_data{
                        .vsm_physical_page_table = data.vsm_physical_page_table.get_bindless(),
                        .prev_buffer = data.prev_buffer.get_bindless(),
                        .next_buffer = data.next_buffer.get_bindless(),
                        .hzb_sampler = hzb_sampler->get_bindless(),
                        .next_size = next_size,
                    });

                command.set_parameter(0, RDG_PARAMETER_BINDLESS);

                command
                    .dispatch_3d(next_size, next_size, VSM_PHYSICAL_PAGE_TABLE_PAGE_COUNT, 8, 8, 1);
            });
    }
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
        rdg_buffer_srv vsm_directional_buffer;
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
                data.vsm_directional_buffer =
                    pass.add_buffer_srv(m_vsm_directional_buffer, RHI_PIPELINE_STAGE_COMPUTE);
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
                        .vsm_directional_buffer = data.vsm_directional_buffer.get_bindless(),
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
}
} // namespace violet
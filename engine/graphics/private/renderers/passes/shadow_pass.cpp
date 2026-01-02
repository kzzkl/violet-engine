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
        std::uint32_t vsm_dispatch_buffer;
        std::uint32_t visible_light_count;
        std::uint32_t lru_state;
        std::uint32_t lru_curr_index;
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
        std::uint32_t vsm_dispatch_buffer;
        std::uint32_t camera_id;
        std::uint32_t directional_vsm_buffer;
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
        std::uint32_t visible_light_count;
        std::uint32_t visible_vsm_ids;
        std::uint32_t virtual_page_table;
        std::uint32_t vsm_buffer;
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
        std::uint32_t virtual_page_table;
        std::uint32_t directional_vsm_buffer;
        std::uint32_t vsm_buffer;
        std::uint32_t render_target;
        std::uint32_t camera_id;
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
        std::uint32_t virtual_page_table;
        std::uint32_t physical_page_table;
        std::uint32_t vsm_buffer;
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
        std::uint32_t physical_page_table;
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
        std::uint32_t physical_page_table;
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
        std::uint32_t physical_page_table;
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
        std::uint32_t visible_light_count;
        std::uint32_t visible_vsm_ids;
        std::uint32_t vsm_buffer;
        std::uint32_t virtual_page_table;
        std::uint32_t physical_page_table;
        std::uint32_t lru_state;
        std::uint32_t lru_buffer;
        std::uint32_t lru_curr_index;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct vsm_debug_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/virtual_shadow_map/debug.hlsl";

    struct constant_data
    {
        std::uint32_t render_target;
        std::uint32_t visible_vsm_ids;
        std::uint32_t virtual_page_table;
        std::uint32_t physical_page_table;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = camera},
    };
};

void shadow_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Shadow Pass");

    m_render_target = parameter.render_target;
    m_depth_buffer = parameter.depth_buffer;

    m_lru_state = parameter.lru_state;
    m_lru_buffer = parameter.lru_buffer;
    m_lru_curr_index = parameter.lru_curr_index;
    m_lru_prev_index = parameter.lru_prev_index;

    const auto& scene = graph.get_scene();

    m_vsm_buffer = graph.add_buffer("VSM Buffer", scene.get_vsm_buffer());
    m_virtual_page_table = graph.add_buffer("VSM Page Table", scene.get_vsm_virtual_page_table());
    m_physical_page_table =
        graph.add_buffer("VSM Physical Page Table", scene.get_vsm_physical_page_table());

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

    m_vsm_dispatch_buffer = graph.add_buffer(
        "VSM Dispatch Buffer",
        sizeof(shader::dispatch_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    prepare(graph);
    light_cull(graph);
    clear_page_table(graph);
    mark_visible_pages(graph);
    mark_resident_pages(graph);
    update_lru(graph);
    allocate_pages(graph);
    add_debug_pass(graph);
}

void shadow_pass::prepare(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_uav vsm_dispatch_buffer;
        rdg_buffer_uav visible_light_count;
        rdg_buffer_uav lru_state;
        std::uint32_t lru_curr_index;
    };

    graph.add_pass<pass_data>(
        "VSM Prepare",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.vsm_dispatch_buffer =
                pass.add_buffer_uav(m_vsm_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_light_count =
                pass.add_buffer_uav(m_visible_light_count, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_state = pass.add_buffer_uav(m_lru_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_curr_index = m_lru_curr_index;
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_prepare_cs>(),
            });

            command.set_constant(
                vsm_prepare_cs::constant_data{
                    .vsm_dispatch_buffer = data.vsm_dispatch_buffer.get_bindless(),
                    .visible_light_count = data.visible_light_count.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
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
        rdg_buffer_uav vsm_dispatch_buffer;
        rdg_buffer_srv directional_vsm_buffer;
        std::uint32_t light_count;
        std::uint32_t camera_id;
    };

    m_directional_vsm_buffer =
        graph.add_buffer("Directional VSM Buffer", graph.get_scene().get_directional_vsm_buffer());

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
            data.vsm_dispatch_buffer =
                pass.add_buffer_uav(m_vsm_dispatch_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.directional_vsm_buffer =
                pass.add_buffer_srv(m_directional_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
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
                    .vsm_dispatch_buffer = data.vsm_dispatch_buffer.get_bindless(),
                    .camera_id = data.camera_id,
                    .directional_vsm_buffer = data.directional_vsm_buffer.get_bindless(),
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
        rdg_buffer_srv visible_light_count;
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_uav virtual_page_table;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_ref vsm_dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Clear Page Table",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visible_light_count =
                pass.add_buffer_srv(m_visible_light_count, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.virtual_page_table =
                pass.add_buffer_uav(m_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_dispatch_buffer = pass.add_buffer(
                m_vsm_dispatch_buffer,
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
                    .visible_light_count = data.visible_light_count.get_bindless(),
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .virtual_page_table = data.virtual_page_table.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.vsm_dispatch_buffer.get_rhi());
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
        rdg_buffer_uav virtual_page_table;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_srv directional_vsm_buffer;
        rdg_texture_uav render_target;
        std::uint32_t camera_id;
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
            data.virtual_page_table =
                pass.add_buffer_uav(m_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.directional_vsm_buffer =
                pass.add_buffer_srv(m_directional_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            data.render_target = pass.add_texture_uav(
                m_render_target,
                RHI_PIPELINE_STAGE_COMPUTE | RHI_PIPELINE_STAGE_TRANSFER);

            data.camera_id = graph.get_camera().get_id();
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
                    .virtual_page_table = data.virtual_page_table.get_bindless(),
                    .directional_vsm_buffer = data.directional_vsm_buffer.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .render_target = data.render_target.get_bindless(),
                    .camera_id = data.camera_id,
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
        rdg_buffer_uav virtual_page_table;
        rdg_buffer_uav physical_page_table;
        rdg_buffer_srv vsm_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Mark Resident Pages",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.virtual_page_table =
                pass.add_buffer_uav(m_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.physical_page_table =
                pass.add_buffer_uav(m_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
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
                    .virtual_page_table = data.virtual_page_table.get_bindless(),
                    .physical_page_table = data.physical_page_table.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(VSM_PHYSICAL_PAGE_COUNT);
        });
}

void shadow_pass::update_lru(render_graph& graph)
{
    rdg_scope scope(graph, "VSM Update LRU");

    rdg_buffer* lru_remap = graph.add_buffer(
        "VSM LRU Remap",
        sizeof(std::uint32_t) * VSM_PHYSICAL_PAGE_COUNT,
        RHI_BUFFER_STORAGE);

    // Mark invalid pages.
    struct mark_invalid_pages_pass_data
    {
        rdg_buffer_uav physical_page_table;
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
            data.physical_page_table =
                pass.add_buffer_uav(m_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
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
                    .physical_page_table = data.physical_page_table.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_prev_index = data.lru_prev_index,
                    .lru_remap = data.lru_remap.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(VSM_PHYSICAL_PAGE_COUNT);
        });

    // Scan invalid pages.
    graph.add_pass<scan_pass>({
        .buffer = lru_remap,
        .size = VSM_PHYSICAL_PAGE_COUNT,
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

            command.dispatch_1d(VSM_PHYSICAL_PAGE_COUNT);
        });

    // Append unused pages.
    struct append_unused_pages_pass_data
    {
        rdg_buffer_uav physical_page_table;
        rdg_buffer_uav lru_state;
        rdg_buffer_uav lru_buffer;
        std::uint32_t lru_curr_index;
    };

    graph.add_pass<append_unused_pages_pass_data>(
        "Append Unused Pages",
        RDG_PASS_COMPUTE,
        [&](append_unused_pages_pass_data& data, rdg_pass& pass)
        {
            data.physical_page_table =
                pass.add_buffer_uav(m_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
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
                    .physical_page_table = data.physical_page_table.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(VSM_PHYSICAL_PAGE_COUNT);
        });
}

void shadow_pass::allocate_pages(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_srv visible_light_count;
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_srv vsm_buffer;
        rdg_buffer_uav virtual_page_table;
        rdg_buffer_uav physical_page_table;
        rdg_buffer_uav lru_state;
        rdg_buffer_srv lru_buffer;
        std::uint32_t lru_curr_index;
        rdg_buffer_ref vsm_dispatch_buffer;
    };

    graph.add_pass<pass_data>(
        "VSM Allocate Pages",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.visible_light_count =
                pass.add_buffer_srv(m_visible_light_count, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.vsm_buffer = pass.add_buffer_srv(m_vsm_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.virtual_page_table =
                pass.add_buffer_uav(m_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.physical_page_table =
                pass.add_buffer_uav(m_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_state = pass.add_buffer_uav(m_lru_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_buffer = pass.add_buffer_srv(m_lru_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.lru_curr_index = m_lru_curr_index;

            data.vsm_dispatch_buffer = pass.add_buffer(
                m_vsm_dispatch_buffer,
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
                    .visible_light_count = data.visible_light_count.get_bindless(),
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .vsm_buffer = data.vsm_buffer.get_bindless(),
                    .virtual_page_table = data.virtual_page_table.get_bindless(),
                    .physical_page_table = data.physical_page_table.get_bindless(),
                    .lru_state = data.lru_state.get_bindless(),
                    .lru_buffer = data.lru_buffer.get_bindless(),
                    .lru_curr_index = data.lru_curr_index,
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_indirect(data.vsm_dispatch_buffer.get_rhi());
        });
}

void shadow_pass::add_debug_pass(render_graph& graph)
{
    struct pass_data
    {
        rdg_texture_uav render_target;
        rdg_buffer_srv visible_vsm_ids;
        rdg_buffer_srv virtual_page_table;
        rdg_buffer_srv physical_page_table;
    };

    graph.add_pass<pass_data>(
        "VSM Debug",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.render_target = pass.add_texture_uav(m_render_target, RHI_PIPELINE_STAGE_COMPUTE);
            data.visible_vsm_ids =
                pass.add_buffer_srv(m_visible_vsm_ids, RHI_PIPELINE_STAGE_COMPUTE);
            data.virtual_page_table =
                pass.add_buffer_srv(m_virtual_page_table, RHI_PIPELINE_STAGE_COMPUTE);
            data.physical_page_table =
                pass.add_buffer_srv(m_physical_page_table, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<vsm_debug_cs>(),
            });

            command.set_constant(
                vsm_debug_cs::constant_data{
                    .render_target = data.render_target.get_bindless(),
                    .visible_vsm_ids = data.visible_vsm_ids.get_bindless(),
                    .virtual_page_table = data.virtual_page_table.get_bindless(),
                    .physical_page_table = data.physical_page_table.get_bindless(),
                });

            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_2d(256, 512);
        });
}
} // namespace violet
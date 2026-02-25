#include "graphics/renderers/passes/cull_pass.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/graphics_config.hpp"

namespace violet
{
struct prepare_cluster_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/prepare_cluster_cull.hlsl";

    struct constant_data
    {
        std::uint32_t cluster_queue_state;
        std::uint32_t dispatch_buffer;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct instance_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/instance_cull.hlsl";

    struct constant_data
    {
        std::uint32_t hzb;
        std::uint32_t hzb_sampler;
        std::uint32_t draw_buffer;
        std::uint32_t draw_count_buffer;
        std::uint32_t draw_info_buffer;
        std::uint32_t cluster_queue;
        std::uint32_t cluster_queue_state;
        std::uint32_t max_draw_command_count;
        std::uint32_t recheck_instances;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
        {.space = 1, .desc = scene},
        {.space = 2, .desc = camera},
    };
};

struct cluster_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/cluster_cull.hlsl";

    struct constant_data
    {
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
        {.space = 2, .desc = camera},
    };
};

void cull_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Cull");

    m_hzb = parameter.hzb;
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

    m_cluster_queue = parameter.cluster_queue;
    m_cluster_queue_state = parameter.cluster_queue_state;

    m_draw_buffer = parameter.draw_buffer;
    m_draw_count_buffer = parameter.draw_count_buffer;
    m_draw_info_buffer = parameter.draw_info_buffer;

    m_recheck_instances = parameter.recheck_instances;

    m_stage = parameter.stage;

    add_prepare_pass(graph);

    add_instance_cull_pass(graph);

    if (parameter.cluster_queue != nullptr && parameter.cluster_queue_state != nullptr)
    {
        rdg_scope cluster_scope(graph, "Cluster Cull");
        add_cluster_cull_pass(graph);
    }
}

void cull_pass::prepare_cluster_cull(
    render_graph& graph,
    rdg_buffer* dispatch_buffer,
    bool cull_cluster_node,
    bool recheck)
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
        [cull_cluster_node, recheck](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines;

            if (cull_cluster_node)
            {
                defines.emplace_back(L"-DCULL_CLUSTER_NODE=1");
            }

            if (recheck)
            {
                defines.emplace_back(L"-DCULL_RECHECK");
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<prepare_cluster_cull_cs>(defines),
            });

            command.set_constant(
                prepare_cluster_cull_cs::constant_data{
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
                    .dispatch_buffer = data.dispatch_buffer.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(1, 1);
        });
}

void cull_pass::add_prepare_pass(render_graph& graph)
{
    struct pass_data
    {
        rdg_buffer_ref count_buffer;
        rdg_buffer_ref cluster_queue_state;
    };

    graph.add_pass<pass_data>(
        "Prepare Parameter",
        RDG_PASS_TRANSFER,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.count_buffer = pass.add_buffer(
                m_draw_count_buffer,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE);

            if (m_cluster_queue_state != nullptr && m_stage == CULL_STAGE_MAIN_PASS)
            {
                data.cluster_queue_state = pass.add_buffer(
                    m_cluster_queue_state,
                    RHI_PIPELINE_STAGE_TRANSFER,
                    RHI_ACCESS_TRANSFER_WRITE);
            }
            else
            {
                data.cluster_queue_state.reset();
            }
        },
        [](const pass_data& data, rdg_command& command)
        {
            command.fill_buffer(
                data.count_buffer.get_rhi(),
                {
                    .offset = 0,
                    .size = data.count_buffer.get_size(),
                },
                0);

            if (data.cluster_queue_state)
            {
                command.fill_buffer(
                    data.cluster_queue_state.get_rhi(),
                    {
                        .offset = 0,
                        .size = data.cluster_queue_state.get_size(),
                    },
                    0);
            }
        });
}

void cull_pass::add_instance_cull_pass(render_graph& graph)
{
    struct pass_data
    {
        rdg_texture_srv hzb;
        rhi_sampler* hzb_sampler;

        rdg_buffer_uav draw_buffer;
        rdg_buffer_uav draw_count_buffer;
        rdg_buffer_uav draw_info_buffer;

        rdg_buffer_uav cluster_queue;
        rdg_buffer_uav cluster_queue_state;

        rdg_buffer_uav recheck_instances_uav;
        rdg_buffer_srv recheck_instances_srv;

        std::uint32_t instance_count;
    };

    graph.add_pass<pass_data>(
        "Instance Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.hzb = pass.add_texture_srv(m_hzb, RHI_PIPELINE_STAGE_COMPUTE);
            data.hzb_sampler = m_hzb_sampler;

            data.draw_buffer = pass.add_buffer_uav(m_draw_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.draw_count_buffer =
                pass.add_buffer_uav(m_draw_count_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.draw_info_buffer =
                pass.add_buffer_uav(m_draw_info_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            if (m_cluster_queue != nullptr && m_cluster_queue_state != nullptr)
            {
                data.cluster_queue =
                    pass.add_buffer_uav(m_cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
                data.cluster_queue_state =
                    pass.add_buffer_uav(m_cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
            }
            else
            {
                data.cluster_queue.reset();
                data.cluster_queue_state.reset();
            }

            if (m_stage == CULL_STAGE_MAIN_PASS)
            {
                data.recheck_instances_uav =
                    pass.add_buffer_uav(m_recheck_instances, RHI_PIPELINE_STAGE_COMPUTE);
                data.recheck_instances_srv.reset();
            }
            else
            {
                data.recheck_instances_uav.reset();
                data.recheck_instances_srv =
                    pass.add_buffer_srv(m_recheck_instances, RHI_PIPELINE_STAGE_COMPUTE);
            }

            data.instance_count = graph.get_scene().get_instance_count();
        },
        [stage = m_stage](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines;
            if (data.cluster_queue)
            {
                defines.emplace_back(L"-DGENERATE_CLUSTER_LIST");
            }

            if (stage == CULL_STAGE_MAIN_PASS)
            {
                defines.emplace_back(L"-DCULL_MAIN_PASS=1");
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<instance_cull_cs>(defines),
            });

            command.set_constant(
                instance_cull_cs::constant_data{
                    .hzb = data.hzb.get_bindless(),
                    .hzb_sampler = data.hzb_sampler->get_bindless(),
                    .draw_buffer = data.draw_buffer.get_bindless(),
                    .draw_count_buffer = data.draw_count_buffer.get_bindless(),
                    .draw_info_buffer = data.draw_info_buffer.get_bindless(),
                    .cluster_queue = data.cluster_queue ? data.cluster_queue.get_bindless() : 0,
                    .cluster_queue_state =
                        data.cluster_queue_state ? data.cluster_queue_state.get_bindless() : 0,
                    .max_draw_command_count = graphics_config::get_max_draw_command_count(),
                    .recheck_instances = stage == CULL_STAGE_MAIN_PASS ?
                                             data.recheck_instances_uav.get_bindless() :
                                             data.recheck_instances_srv.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.dispatch_1d(data.instance_count);
        });
}

void cull_pass::add_cluster_cull_pass(render_graph& graph)
{
    rdg_buffer* dispatch_buffer = graph.add_buffer(
        "Dispatch Buffer",
        3 * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    struct pass_data
    {
        rdg_texture_srv hzb;
        rhi_sampler* hzb_sampler;

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
        bool cull_cluster_node = i != cluster_node_depth - 1;
        bool recheck = m_stage == CULL_STAGE_POST_PASS && i == 0;

        prepare_cluster_cull(graph, dispatch_buffer, cull_cluster_node, recheck);

        graph.add_pass<pass_data>(
            cull_cluster_node ? "Cull Cluster Node: " + std::to_string(i) : "Cull Cluster",
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.hzb = pass.add_texture_srv(m_hzb, RHI_PIPELINE_STAGE_COMPUTE);
                data.hzb_sampler = m_hzb_sampler;

                data.cluster_queue =
                    pass.add_buffer_uav(m_cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
                data.cluster_queue_state =
                    pass.add_buffer_uav(m_cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
                data.dispatch_buffer = pass.add_buffer(
                    dispatch_buffer,
                    RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                    RHI_ACCESS_INDIRECT_COMMAND_READ);

                if (!cull_cluster_node)
                {
                    data.draw_buffer =
                        pass.add_buffer_uav(m_draw_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                    data.draw_count_buffer =
                        pass.add_buffer_uav(m_draw_count_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                    data.draw_info_buffer =
                        pass.add_buffer_uav(m_draw_info_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                }
            },
            [=, stage = m_stage](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                std::vector<std::wstring> defines;

                if (stage == CULL_STAGE_MAIN_PASS)
                {
                    defines.emplace_back(L"-DCULL_MAIN_PASS=1");
                }

                if (cull_cluster_node)
                {
                    defines.emplace_back(L"-DCULL_CLUSTER_NODE=1");
                }

                if (recheck)
                {
                    defines.emplace_back(L"-DCULL_RECHECK=1");
                }

                command.set_pipeline({
                    .compute_shader = device.get_shader<cluster_cull_cs>(defines),
                });

                cluster_cull_cs::constant_data constant = {
                    .hzb = data.hzb.get_bindless(),
                    .hzb_sampler = data.hzb_sampler->get_bindless(),
                    .threshold = 1.0f,
                    .cluster_queue = data.cluster_queue.get_bindless(),
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
                    .max_cluster_count = graphics_config::get_max_cluster_count(),
                    .max_cluster_node_count = graphics_config::get_max_cluster_node_count(),
                    .max_draw_command_count = graphics_config::get_max_draw_command_count(),
                };

                if (cull_cluster_node)
                {
                    rhi_buffer_srv* cluster_node_buffer_srv =
                        device.get_geometry_manager()->get_cluster_node_buffer()->get_srv();
                    constant.cluster_node_buffer = cluster_node_buffer_srv->get_bindless();
                }
                else
                {
                    constant.draw_buffer = data.draw_buffer.get_bindless();
                    constant.draw_count_buffer = data.draw_count_buffer.get_bindless();
                    constant.draw_info_buffer = data.draw_info_buffer.get_bindless();
                }

                command.set_constant(constant);
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_SCENE);
                command.set_parameter(2, RDG_PARAMETER_CAMERA);

                command.dispatch_indirect(data.dispatch_buffer.get_rhi(), 0);
            });
    }
}
} // namespace violet
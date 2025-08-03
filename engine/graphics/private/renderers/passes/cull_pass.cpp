#include "graphics/renderers/passes/cull_pass.hpp"
#include "graphics/geometry_manager.hpp"
#include "graphics/graphics_config.hpp"
#include "graphics/renderers/features/cluster_render_feature.hpp"

namespace violet
{
namespace
{
vec4f get_frustum(const mat4x4f& matrix_p)
{
    mat4f matrix_p_t = matrix::transpose(matrix_p);
    vec4f frustum_x = vector::normalize(matrix_p_t[3] + matrix_p_t[0]);
    frustum_x /= vector::length(vec3f(frustum_x));
    vec4f frustum_y = vector::normalize(matrix_p_t[3] + matrix_p_t[0]);
    frustum_y /= vector::length(vec3f(frustum_y));

    return vec4f(frustum_x.x, frustum_x.z, frustum_y.y, frustum_y.z);
}
} // namespace

struct prepare_cluster_queue_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/prepare_cluster_queue.hlsl";

    struct constant_data
    {
        std::uint32_t cluster_queue;
        std::uint32_t cluster_queue_size;
    };

    static constexpr parameter_layout parameters = {
        {.space = 0, .desc = bindless},
    };
};

struct prepare_cluster_cull_cs : public shader_cs
{
    static constexpr std::string_view path = "assets/shaders/cull/prepare_cluster_cull.hlsl";

    struct constant_data
    {
        std::uint32_t cluster_queue_state;
        std::uint32_t dispatch_command_buffer;
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
        std::uint32_t hzb_width;
        std::uint32_t hzb_height;
        vec4f frustum;
        std::uint32_t command_buffer;
        std::uint32_t count_buffer;
        std::uint32_t cluster_queue;
        std::uint32_t cluster_queue_state;
        std::uint32_t max_draw_commands;
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
        std::uint32_t hzb_width;
        std::uint32_t hzb_height;
        vec4f frustum;

        float threshold;
        float lod_scale;

        std::uint32_t cluster_buffer;
        std::uint32_t cluster_node_buffer;
        std::uint32_t cluster_queue;
        std::uint32_t cluster_queue_state;

        std::uint32_t command_buffer;
        std::uint32_t count_buffer;

        std::uint32_t max_clusters;
        std::uint32_t max_cluster_nodes;
        std::uint32_t max_draw_commands;
    };

    static constexpr parameter_layout parameters = {
        {0, bindless},
        {1, scene},
        {2, camera},
    };
};

cull_pass::output cull_pass::add(render_graph& graph, const parameter& parameter)
{
    rdg_scope scope(graph, "Cull");

    cull_data cull_data = {
        .hzb = parameter.hzb,
        .hzb_sampler = render_device::instance().get_sampler({
            .mag_filter = RHI_FILTER_LINEAR,
            .min_filter = RHI_FILTER_LINEAR,
            .address_mode_u = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_mode_v = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_mode_w = RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .min_level = 0.0f,
            .max_level = -1.0f,
            .reduction_mode = RHI_SAMPLER_REDUCTION_MODE_MIN,
        }),
        .frustum = get_frustum(graph.get_camera().get_matrix_p()),
    };

    cull_data.command_buffer = graph.add_buffer(
        "Command Buffer",
        graph.get_scene().get_instance_capacity() * sizeof(shader::draw_command),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);
    cull_data.count_buffer = graph.add_buffer(
        "Count Buffer",
        graph.get_scene().get_batch_capacity() * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT | RHI_BUFFER_TRANSFER_DST);

    bool use_cluster = parameter.cluster_feature != nullptr &&
                       render_device::instance().get_geometry_manager()->get_cluster_count() != 0;

    if (use_cluster)
    {
        cull_data.cluster_queue =
            graph.add_buffer("Cluster Queue", parameter.cluster_feature->get_cluster_queue());
        cull_data.cluster_queue_state = graph.add_buffer(
            "Cluster Queue State",
            parameter.cluster_feature->get_cluster_queue_state());

        if (parameter.cluster_feature->need_initialize())
        {
            prepare_cluster_queue(graph, cull_data.cluster_queue);
        }
    }

    add_prepare_pass(graph, cull_data.count_buffer, cull_data.cluster_queue_state);

    add_instance_cull_pass(graph, cull_data);

    if (use_cluster)
    {
        rdg_scope cluster_scope(graph, "Cluster Cull");

        if (parameter.cluster_feature->use_persistent_thread())
        {
            add_cluster_cull_pass_persistent(
                graph,
                cull_data,
                parameter.cluster_feature->get_max_clusters(),
                parameter.cluster_feature->get_max_cluster_nodes());
        }
        else
        {
            add_cluster_cull_pass(
                graph,
                cull_data,
                parameter.cluster_feature->get_max_clusters(),
                parameter.cluster_feature->get_max_cluster_nodes());
        }
    }

    return {
        .command_buffer = cull_data.command_buffer,
        .count_buffer = cull_data.count_buffer,
    };
}

void cull_pass::prepare_cluster_queue(render_graph& graph, rdg_buffer* cluster_queue)
{
    struct pass_data
    {
        rdg_buffer_uav cluster_queue;
    };

    graph.add_pass<pass_data>(
        "Prepare Cluster Queue",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.cluster_queue = pass.add_buffer_uav(cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [=](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<prepare_cluster_queue_cs>(),
            });

            auto cluster_queue_size =
                static_cast<std::uint32_t>(data.cluster_queue.get_size() / sizeof(vec2u));

            command.set_constant(
                prepare_cluster_queue_cs::constant_data{
                    .cluster_queue = data.cluster_queue.get_bindless(),
                    .cluster_queue_size = cluster_queue_size,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(cluster_queue_size);
        });
}

void cull_pass::prepare_cluster_cull(
    render_graph& graph,
    rdg_buffer* cluster_queue_state,
    rdg_buffer* dispatch_command_buffer,
    bool cull_cluster)
{
    struct pass_data
    {
        rdg_buffer_uav cluster_queue_state;
        rdg_buffer_uav dispatch_command_buffer;
    };

    graph.add_pass<pass_data>(
        "Prepare Cluster Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.cluster_queue_state =
                pass.add_buffer_uav(cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
            data.dispatch_command_buffer =
                pass.add_buffer_uav(dispatch_command_buffer, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [cull_cluster](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines = {
                cull_cluster ? L"-DCULL_CLUSTER" : L"-DCULL_CLUSTER_NODE",
            };

            command.set_pipeline({
                .compute_shader = device.get_shader<prepare_cluster_cull_cs>(defines),
            });

            command.set_constant(
                prepare_cluster_cull_cs::constant_data{
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
                    .dispatch_command_buffer = data.dispatch_command_buffer.get_bindless(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);

            command.dispatch_1d(1, 1);
        });
}

void cull_pass::add_prepare_pass(
    render_graph& graph,
    rdg_buffer* count_buffer,
    rdg_buffer* cluster_queue_state)
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
                count_buffer,
                RHI_PIPELINE_STAGE_TRANSFER,
                RHI_ACCESS_TRANSFER_WRITE);

            if (cluster_queue_state != nullptr)
            {
                data.cluster_queue_state = pass.add_buffer(
                    cluster_queue_state,
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
            command.fill_buffer(data.count_buffer.get_rhi(), {0, data.count_buffer.get_size()}, 0);

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

void cull_pass::add_instance_cull_pass(render_graph& graph, const cull_data& cull_data)
{
    struct pass_data
    {
        rdg_texture_srv hzb;
        rhi_sampler* hzb_sampler;
        vec4f frustum;

        rdg_buffer_uav command_buffer;
        rdg_buffer_uav count_buffer;
        rdg_buffer_uav cluster_queue;
        rdg_buffer_uav cluster_queue_state;

        std::uint32_t instance_count;
    };

    graph.add_pass<pass_data>(
        "Instance Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.frustum = cull_data.frustum;
            data.hzb = pass.add_texture_srv(cull_data.hzb, RHI_PIPELINE_STAGE_COMPUTE);
            data.hzb_sampler = cull_data.hzb_sampler;

            data.command_buffer =
                pass.add_buffer_uav(cull_data.command_buffer, RHI_PIPELINE_STAGE_COMPUTE);
            data.count_buffer =
                pass.add_buffer_uav(cull_data.count_buffer, RHI_PIPELINE_STAGE_COMPUTE);

            if (cull_data.cluster_queue != nullptr && cull_data.cluster_queue_state != nullptr)
            {
                data.cluster_queue =
                    pass.add_buffer_uav(cull_data.cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
                data.cluster_queue_state =
                    pass.add_buffer_uav(cull_data.cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
            }
            else
            {
                data.cluster_queue.reset();
                data.cluster_queue_state.reset();
            }

            data.instance_count = graph.get_scene().get_instance_count();
        },
        [](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            std::vector<std::wstring> defines;
            if (data.cluster_queue)
            {
                defines.emplace_back(L"-DGENERATE_CLUSTER_LIST");
            }

            command.set_pipeline({
                .compute_shader = device.get_shader<instance_cull_cs>(defines),
            });

            rhi_texture_extent extent = data.hzb.get_extent();

            command.set_constant(
                instance_cull_cs::constant_data{
                    .hzb = data.hzb.get_bindless(),
                    .hzb_sampler = data.hzb_sampler->get_bindless(),
                    .hzb_width = extent.width,
                    .hzb_height = extent.height,
                    .frustum = data.frustum,
                    .command_buffer = data.command_buffer.get_bindless(),
                    .count_buffer = data.count_buffer.get_bindless(),
                    .cluster_queue = data.cluster_queue ? data.cluster_queue.get_bindless() : 0,
                    .cluster_queue_state =
                        data.cluster_queue_state ? data.cluster_queue_state.get_bindless() : 0,
                    .max_draw_commands = graphics_config::get_max_draw_commands(),
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.dispatch_1d(data.instance_count);
        });
}

void cull_pass::add_cluster_cull_pass(
    render_graph& graph,
    const cull_data& cull_data,
    std::uint32_t max_clusters,
    std::uint32_t max_cluster_nodes)
{
    rdg_buffer* dispatch_command_buffer = graph.add_buffer(
        "Dispatch Command Buffer",
        3 * sizeof(std::uint32_t),
        RHI_BUFFER_STORAGE | RHI_BUFFER_INDIRECT);

    struct pass_data
    {
        rdg_texture_srv hzb;
        rhi_sampler* hzb_sampler;
        vec4f frustum;

        rdg_buffer_uav cluster_queue;
        rdg_buffer_uav cluster_queue_state;
        rdg_buffer_uav command_buffer;
        rdg_buffer_uav count_buffer;
        rdg_buffer_ref dispatch_command_buffer;
    };

    const auto& camera = graph.get_camera();
    float lod_scale = static_cast<float>(camera.get_render_target()->get_extent().height) * 0.5f /
                      std::tan(camera.get_fov() * 0.5f);

    std::uint32_t cluster_node_depth =
        render_device::instance().get_geometry_manager()->get_cluster_node_depth();
    for (std::uint32_t i = 0; i < cluster_node_depth; ++i)
    {
        bool cull_cluster = i == cluster_node_depth - 1;

        prepare_cluster_cull(
            graph,
            cull_data.cluster_queue_state,
            dispatch_command_buffer,
            cull_cluster);

        graph.add_pass<pass_data>(
            cull_cluster ? "Cull Cluster" : "Cull Cluster Node: " + std::to_string(i),
            RDG_PASS_COMPUTE,
            [&](pass_data& data, rdg_pass& pass)
            {
                data.hzb = pass.add_texture_srv(cull_data.hzb, RHI_PIPELINE_STAGE_COMPUTE);
                data.hzb_sampler = cull_data.hzb_sampler;
                data.frustum = cull_data.frustum;

                data.cluster_queue =
                    pass.add_buffer_uav(cull_data.cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
                data.cluster_queue_state =
                    pass.add_buffer_uav(cull_data.cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
                data.dispatch_command_buffer = pass.add_buffer(
                    dispatch_command_buffer,
                    RHI_PIPELINE_STAGE_DRAW_INDIRECT,
                    RHI_ACCESS_INDIRECT_COMMAND_READ);

                if (cull_cluster)
                {
                    data.command_buffer =
                        pass.add_buffer_uav(cull_data.command_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                    data.count_buffer =
                        pass.add_buffer_uav(cull_data.count_buffer, RHI_PIPELINE_STAGE_COMPUTE);
                }
            },
            [=](const pass_data& data, rdg_command& command)
            {
                auto& device = render_device::instance();

                std::vector<std::wstring> defines = {
                    cull_cluster ? L"-DCULL_CLUSTER" : L"-DCULL_CLUSTER_NODE",
                };

                command.set_pipeline({
                    .compute_shader = device.get_shader<cluster_cull_cs>(defines),
                });

                rhi_texture_extent extent = data.hzb.get_extent();

                cluster_cull_cs::constant_data constant = {
                    .hzb = data.hzb.get_bindless(),
                    .hzb_sampler = data.hzb_sampler->get_bindless(),
                    .hzb_width = extent.width,
                    .hzb_height = extent.height,
                    .frustum = data.frustum,
                    .threshold = 1.0f,
                    .lod_scale = lod_scale,
                    .cluster_queue = data.cluster_queue.get_bindless(),
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
                    .max_clusters = max_clusters,
                    .max_cluster_nodes = max_cluster_nodes,
                    .max_draw_commands = graphics_config::get_max_draw_commands(),
                };

                if (cull_cluster)
                {
                    rhi_buffer_srv* cluster_buffer_srv =
                        device.get_geometry_manager()->get_cluster_buffer()->get_srv();

                    constant.cluster_buffer = cluster_buffer_srv->get_bindless(),
                    constant.command_buffer = data.command_buffer.get_bindless();
                    constant.count_buffer = data.count_buffer.get_bindless();
                }
                else
                {
                    rhi_buffer_srv* cluster_node_buffer_srv =
                        device.get_geometry_manager()->get_cluster_node_buffer()->get_srv();

                    constant.cluster_node_buffer = cluster_node_buffer_srv->get_bindless();
                }

                command.set_constant(constant);
                command.set_parameter(0, RDG_PARAMETER_BINDLESS);
                command.set_parameter(1, RDG_PARAMETER_SCENE);
                command.set_parameter(2, RDG_PARAMETER_CAMERA);

                command.dispatch_indirect(data.dispatch_command_buffer.get_rhi(), 0);
            });
    }
}

void cull_pass::add_cluster_cull_pass_persistent(
    render_graph& graph,
    const cull_data& cull_data,
    std::uint32_t max_clusters,
    std::uint32_t max_cluster_nodes)
{
    struct pass_data
    {
        rdg_buffer_uav cluster_queue;
        rdg_buffer_uav cluster_queue_state;
    };

    graph.add_pass<pass_data>(
        "Cluster Cull",
        RDG_PASS_COMPUTE,
        [&](pass_data& data, rdg_pass& pass)
        {
            data.cluster_queue =
                pass.add_buffer_uav(cull_data.cluster_queue, RHI_PIPELINE_STAGE_COMPUTE);
            data.cluster_queue_state =
                pass.add_buffer_uav(cull_data.cluster_queue_state, RHI_PIPELINE_STAGE_COMPUTE);
        },
        [=](const pass_data& data, rdg_command& command)
        {
            auto& device = render_device::instance();

            command.set_pipeline({
                .compute_shader = device.get_shader<cluster_cull_cs>(),
            });

            rhi_buffer_srv* cluster_buffer_srv =
                device.get_geometry_manager()->get_cluster_buffer()->get_srv();
            rhi_buffer_srv* cluster_node_buffer_srv =
                device.get_geometry_manager()->get_cluster_node_buffer()->get_srv();
            command.set_constant(
                cluster_cull_cs::constant_data{
                    .cluster_buffer = cluster_buffer_srv->get_bindless(),
                    .cluster_node_buffer = cluster_node_buffer_srv->get_bindless(),
                    .cluster_queue = data.cluster_queue.get_bindless(),
                    .cluster_queue_state = data.cluster_queue_state.get_bindless(),
                    .max_clusters = max_clusters,
                    .max_cluster_nodes = max_cluster_nodes,
                });
            command.set_parameter(0, RDG_PARAMETER_BINDLESS);
            command.set_parameter(1, RDG_PARAMETER_SCENE);
            command.set_parameter(2, RDG_PARAMETER_CAMERA);

            command.dispatch_1d(1, 64);
        });
}
} // namespace violet
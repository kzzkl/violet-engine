#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "control/control_module.hpp"
#include "core/engine.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/graphics_module.hpp"
#include "graphics/passes/present_pass.hpp"
#include "graphics/passes/skybox_pass.hpp"
#include "scene/scene_module.hpp"
#include "window/window_module.hpp"
#include <fstream>
#include <thread>

namespace violet::sample
{
class sample_pass : public rdg_render_pass
{
public:
    static constexpr std::size_t reference_render_target{0};
    static constexpr std::size_t reference_depth{1};

public:
    sample_pass()
    {
        add_color(reference_render_target, RHI_TEXTURE_LAYOUT_RENDER_TARGET);
        add_depth_stencil(reference_depth, RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

        set_shader(
            "./hello-world/shaders/sample.vert.spv",
            "./hello-world/shaders/sample.frag.spv");

        set_primitive_topology(RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

        set_input_layout({
            {"position", RHI_FORMAT_R32G32B32_FLOAT}
        });

        set_parameter_layout({
            {engine_parameter_layout::mesh,   RDG_PASS_PARAMETER_FLAG_NONE},
            {engine_parameter_layout::camera, RDG_PASS_PARAMETER_FLAG_NONE}
        });
    }

    virtual void execute(rhi_command* command, rdg_context* context) override
    {
        rhi_texture_extent extent =
            context->get_texture(this, reference_render_target)->get_extent();

        rhi_viewport viewport = {};
        viewport.width = extent.width;
        viewport.height = extent.height;
        viewport.min_depth = 0.0f;
        viewport.max_depth = 1.0f;
        command->set_viewport(viewport);

        rhi_scissor_rect scissor = {};
        scissor.max_x = extent.width;
        scissor.max_y = extent.height;
        command->set_scissor(&scissor, 1);

        command->set_render_pipeline(get_pipeline());
        command->set_render_parameter(1, context->get_camera());
        for (const rdg_mesh& mesh : context->get_meshes(this))
        {
            command->set_render_parameter(0, mesh.transform);
            command->set_vertex_buffers(mesh.vertex_buffers, get_input_layout().size());
            command->set_index_buffer(mesh.index_buffer);
            command->draw_indexed(mesh.index_start, mesh.index_count, mesh.vertex_start);
        }
    }

private:
    rdg_pass_reference* m_color;
};

class hello_world : public engine_module
{
public:
    hello_world() : engine_module("hello_world") {}

    virtual bool initialize(const dictionary& config) override
    {
        log::info(config["text"]);

        auto& window = get_module<window_module>();
        window.on_resize().then(
            [this](std::uint32_t width, std::uint32_t height)
            {
                resize(width, height);
            });

        initialize_render();
        initialize_scene();

        on_tick().then(
            [this](float delta)
            {
                tick(delta);
            });

        return true;
    }

    virtual void shutdown() override {}

private:
    void initialize_render()
    {
        auto& window = get_module<window_module>();
        auto& graphics = get_module<graphics_module>();

        render_device* device = graphics.get_device();

        auto window_extent = window.get_extent();
        m_swapchain = device->create_swapchain(
            rhi_swapchain_desc{window_extent.width, window_extent.height, window.get_handle()});

        m_render_graph = std::make_unique<render_graph>();

        rdg_texture* render_target =
            m_render_graph->add_resource<rdg_texture>("render target", true);
        render_target->set_format(m_swapchain->get_texture()->get_format());

        rdg_texture* depth_buffer = m_render_graph->add_resource<rdg_texture>("depth buffer", true);
        depth_buffer->set_format(RHI_FORMAT_D24_UNORM_S8_UINT);

        sample_pass* mesh_pass = m_render_graph->add_pass<sample_pass>("mesh pass");
        skybox_pass* skybox = m_render_graph->add_pass<skybox_pass>("skybox pass");
        present_pass* present = m_render_graph->add_pass<present_pass>("present pass");

        m_render_graph->add_edge(
            render_target,
            mesh_pass,
            sample_pass::reference_render_target,
            RDG_EDGE_ACTION_CLEAR);
        m_render_graph->add_edge(
            depth_buffer,
            mesh_pass,
            sample_pass::reference_depth,
            RDG_EDGE_ACTION_CLEAR);
        m_render_graph->add_edge(
            mesh_pass,
            sample_pass::reference_render_target,
            skybox,
            skybox_pass::reference_render_target,
            RDG_EDGE_ACTION_LOAD);
        m_render_graph->add_edge(
            mesh_pass,
            sample_pass::reference_depth,
            skybox,
            skybox_pass::reference_depth,
            RDG_EDGE_ACTION_LOAD);
        m_render_graph->add_edge(
            skybox,
            skybox_pass::reference_render_target,
            present,
            present_pass::reference_present_target,
            RDG_EDGE_ACTION_LOAD);

        m_render_graph->compile(device);

        m_material_layout = std::make_unique<material_layout>(
            m_render_graph.get(),
            std::vector<rdg_pass*>{mesh_pass});
        m_material = std::make_unique<material>(device, m_material_layout.get());

        m_main_camera = std::make_unique<actor>("camera", get_world());
        auto [camera_transform, main_camera, camera_control] =
            m_main_camera->add<transform, camera, orbit_control>();
        camera_transform->set_position(float3{0.0f, 0.0f, -10.0f});
        main_camera->set_render_graph(m_render_graph.get());

        m_skybox = device->create_texture_cube(
            "hello-world/skybox/icebergs/right.jpg",
            "hello-world/skybox/icebergs/left.jpg",
            "hello-world/skybox/icebergs/top.jpg",
            "hello-world/skybox/icebergs/bottom.jpg",
            "hello-world/skybox/icebergs/front.jpg",
            "hello-world/skybox/icebergs/back.jpg");

        rhi_sampler_desc sampler_desc = {};
        sampler_desc.min_filter = RHI_FILTER_LINEAR;
        sampler_desc.mag_filter = RHI_FILTER_LINEAR;
        sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
        m_skybox_sampler = device->create_sampler(sampler_desc);

        main_camera->set_skybox(m_skybox.get(), m_skybox_sampler.get());
        main_camera->set_skybox(m_skybox.get(), m_skybox_sampler.get());

        resize(window_extent.width, window_extent.height);
    }

    void initialize_scene()
    {
        m_geometry = std::make_unique<box_geometry>(get_module<graphics_module>().get_device());

        m_cube = std::make_unique<actor>("cube", get_world());
        auto [cube_transform, cube_mesh] = m_cube->add<transform, mesh>();
        cube_mesh->set_geometry(m_geometry.get());
        cube_mesh->add_submesh(0, 0, 0, m_geometry->get_index_count(), m_material.get());
    }

    void tick(float delta)
    {
        static float time = 0;
        float scale = std::abs(std::sin(time));
        time += delta;
        m_cube->get<transform>()->set_scale(float3{scale, 1.0f, 1.0f});

        return;
        auto& window = get_module<window_module>();
        auto rect = window.get_extent();

        if (rect.width == 0 || rect.height == 0)
            return;

        matrix4 p = matrix_simd::perspective(
            to_radians(45.0f),
            static_cast<float>(rect.width) / static_cast<float>(rect.height),
            0.1f,
            100.0f);

        matrix4 m = matrix_simd::affine_transform(
            simd::set(10.0, 10.0, 10.0, 0.0),
            quaternion_simd::rotation_axis(simd::set(1.0f, 0.0f, 0.0f, 0.0f), m_rotate),
            simd::set(0.0, 0.0, 0.0, 0.0));

        matrix4 v = matrix_simd::affine_transform(
            simd::set(1.0f, 1.0f, 1.0f, 0.0f),
            simd::set(0.0f, 0.0f, 0.0f, 1.0f),
            simd::set(0.0, 0.0, -30.0f, 0.0f));
        v = matrix_simd::inverse_transform(v);

        matrix4 mvp = matrix_simd::mul(matrix_simd::mul(m, v), p);

        auto cube_transform = m_cube->get<transform>();
        cube_transform->set_rotation(
            quaternion_simd::rotation_axis(simd::set(1.0f, 0.0f, 0.0f, 0.0f), m_rotate));

        m_rotate += delta * 2.0f;
    }

    void resize(std::uint32_t width, std::uint32_t height)
    {
        auto& graphics = get_module<graphics_module>();

        m_swapchain->resize(width, height);

        rhi_texture_desc depth_desc = {};
        depth_desc.width = width;
        depth_desc.height = height;
        depth_desc.format = RHI_FORMAT_D24_UNORM_S8_UINT;
        depth_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_desc.flags = RHI_TEXTURE_FLAG_DEPTH_STENCIL;
        m_depth = graphics.get_device()->create_texture(depth_desc);

        auto main_camera = m_main_camera->get<camera>();
        main_camera->resize(width, height);
        main_camera->set_render_texture("render target", m_swapchain.get());
        main_camera->set_render_texture("depth buffer", m_depth.get());
    }

    rhi_ptr<rhi_swapchain> m_swapchain;
    rhi_ptr<rhi_texture> m_depth;
    rhi_ptr<rhi_texture> m_skybox;
    rhi_ptr<rhi_sampler> m_skybox_sampler;
    std::unique_ptr<render_graph> m_render_graph;

    std::unique_ptr<material_layout> m_material_layout;
    std::unique_ptr<material> m_material;

    std::unique_ptr<geometry> m_geometry;
    std::unique_ptr<actor> m_cube;
    std::unique_ptr<actor> m_main_camera;

    float m_rotate = 0.0f;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine engine;
    engine.initialize("hello-world/config");
    engine.install<window_module>();
    engine.install<scene_module>();
    engine.install<graphics_module>();
    engine.install<control_module>();
    engine.install<sample::hello_world>();

    engine.get_module<window_module>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();

    return 0;
}
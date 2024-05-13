#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "control/control_system.hpp"
#include "core/engine.hpp"
#include "graphics/geometries/box_geometry.hpp"
#include "graphics/graphics_system.hpp"
#include "scene/scene_system.hpp"
#include "window/window_system.hpp"
#include <filesystem>
#include <fstream>
#include <thread>

namespace violet::sample
{
class sample_pass : public mesh_pass
{
public:
    sample_pass()
    {
        add_color("color", RHI_TEXTURE_LAYOUT_RENDER_TARGET);
        add_depth_stencil("depth", RHI_TEXTURE_LAYOUT_DEPTH_STENCIL);

        set_vertex_shader("./hello-world/shaders/sample.vert.spv");
        set_fragment_shader("./hello-world/shaders/sample.frag.spv");

        set_primitive_topology(RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    }

    virtual std::vector<rhi_parameter_desc> get_parameter_layout() const override
    {
        return {parameter_layout::mesh, parameter_layout::camera};
    };

    virtual void execute(rhi_render_command* command, render_context* context) override
    {
        rhi_texture_extent extent = get_reference("color")->resource->get_texture()->get_extent();

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

        for (const render_mesh* mesh : get_meshes())
        {
            command->set_render_parameter(0, mesh->transform);
            command->set_vertex_buffers(mesh->vertex_buffers.data(), mesh->vertex_buffers.size());
            command->set_index_buffer(mesh->index_buffer);
            command->draw_indexed(mesh->index_start, mesh->index_count, mesh->vertex_start);
        }

        clear_mesh();
    }

private:
};

class present_pass : public pass
{
public:
    present_pass()
    {
        add_texture(
            "target",
            RHI_ACCESS_FLAG_SHADER_READ | RHI_ACCESS_FLAG_SHADER_WRITE,
            RHI_TEXTURE_LAYOUT_PRESENT);
    }
};

class hello_world : public engine_system
{
public:
    hello_world() : engine_system("hello_world") {}

    virtual bool initialize(const dictionary& config) override
    {
        log::info(config["text"]);

        auto& window = get_system<window_system>();
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
                get_system<graphics_system>().render(m_render_graph.get());
            });

        return true;
    }

    virtual void shutdown() override {}

private:
    void initialize_render()
    {
        auto& window = get_system<window_system>();
        auto& graphics = get_system<graphics_system>();

        auto window_extent = window.get_extent();
        m_swapchain = graphics.get_renderer()->create_swapchain(
            rhi_swapchain_desc{window_extent.width, window_extent.height, window.get_handle()});

        m_render_graph = std::make_unique<render_graph>(graphics.get_renderer());

        swapchain* render_target = m_render_graph->add_resource<swapchain>("render target");
        render_target->set_format(m_swapchain->get_texture()->get_format());

        texture* depth_buffer = m_render_graph->add_resource<texture>("depth buffer");
        depth_buffer->set_format(RHI_FORMAT_D24_UNORM_S8_UINT);

        sample_pass* mesh_pass = m_render_graph->add_pass<sample_pass>("mesh pass");
        present_pass* present = m_render_graph->add_pass<present_pass>("present pass");

        m_render_graph->add_edge(render_target, mesh_pass, "color", EDGE_OPERATE_CLEAR);
        m_render_graph->add_edge(depth_buffer, mesh_pass, "depth", EDGE_OPERATE_CLEAR);
        m_render_graph->add_edge(mesh_pass, "color", present, "target", EDGE_OPERATE_STORE);

        m_render_graph->compile();

        m_render_graph->add_material_layout("test material", {mesh_pass});
        m_material = m_render_graph->add_material("test material", "test material");

        resize(window_extent.width, window_extent.height);
    }

    void initialize_scene()
    {
        m_geometry = std::make_unique<box_geometry>(get_system<graphics_system>().get_renderer());

        m_cube = std::make_unique<actor>("cube", get_world());
        auto [cube_transform, cube_mesh] = m_cube->add<transform, mesh>();
        cube_mesh->set_geometry(m_geometry.get());
        cube_mesh->add_submesh(0, 0, 0, m_geometry->get_index_count(), m_material);

        m_main_camera = std::make_unique<actor>("camera", get_world());
        auto [camera_transform, main_camera] = m_main_camera->add<transform, camera>();
    }

    void tick(float delta)
    {
        static float time = 0;
        float scale = std::abs(std::sin(time));
        time += delta;
        m_cube->get<transform>()->set_scale(float3{scale, 1.0f, 1.0f});

        return;
        auto& window = get_system<window_system>();
        auto rect = window.get_extent();

        if (rect.width == 0 || rect.height == 0)
            return;

        float4x4_simd p = matrix_simd::perspective(
            to_radians(45.0f),
            static_cast<float>(rect.width) / static_cast<float>(rect.height),
            0.1f,
            100.0f);

        float4x4_simd m = matrix_simd::affine_transform(
            simd::set(10.0, 10.0, 10.0, 0.0),
            quaternion_simd::rotation_axis(simd::set(1.0f, 0.0f, 0.0f, 0.0f), m_rotate),
            simd::set(0.0, 0.0, 0.0, 0.0));

        float4x4_simd v = matrix_simd::affine_transform(
            simd::set(1.0f, 1.0f, 1.0f, 0.0f),
            simd::set(0.0f, 0.0f, 0.0f, 1.0f),
            simd::set(0.0, 0.0, -30.0f, 0.0f));
        v = matrix_simd::inverse_transform(v);

        float4x4_simd mvp = matrix_simd::mul(matrix_simd::mul(m, v), p);

        auto cube_transform = m_cube->get<transform>();
        cube_transform->set_rotation(
            quaternion_simd::rotation_axis(simd::set(1.0f, 0.0f, 0.0f, 0.0f), m_rotate));

        m_rotate += delta * 2.0f;
    }

    void resize(std::uint32_t width, std::uint32_t height)
    {
        auto& graphics = get_system<graphics_system>();

        m_swapchain->resize(width, height);

        rhi_texture_desc depth_desc = {};
        depth_desc.width = width;
        depth_desc.height = height;
        depth_desc.format = RHI_FORMAT_D24_UNORM_S8_UINT;
        depth_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_desc.flags = RHI_TEXTURE_FLAG_DEPTH_STENCIL;
        m_depth = graphics.get_renderer()->create_texture(depth_desc);

        m_render_graph->get_resource<swapchain>("render target")->set(m_swapchain.get());
        m_render_graph->get_resource<texture>("depth buffer")->set(m_depth.get());
    }

    rhi_ptr<rhi_swapchain> m_swapchain;
    rhi_ptr<rhi_texture> m_depth;
    std::unique_ptr<render_graph> m_render_graph;

    std::unique_ptr<geometry> m_geometry;
    material* m_material;
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
    engine.install<window_system>();
    engine.install<scene_system>();
    engine.install<graphics_system>();
    engine.install<control_system>();
    engine.install<sample::hello_world>();

    engine.get_system<window_system>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();

    return 0;
}
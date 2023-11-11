#include "common/log.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "control/control_system.hpp"
#include "core/ecs/actor.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/pipeline/debug_pipeline.hpp"
#include "physics/physics_system.hpp"
#include "physics/physics_world.hpp"
#include "scene/scene_system.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class color_pipeline : public render_pipeline
{
public:
    color_pipeline(std::string_view name, graphics_context* context)
        : render_pipeline(name, context)
    {
        set_shader("physics/shaders/basic.vert.spv", "physics/shaders/basic.frag.spv");
        set_vertex_attributes({
            {"position", RHI_RESOURCE_FORMAT_R32G32B32_FLOAT},
            {"color",    RHI_RESOURCE_FORMAT_R32G32B32_FLOAT}
        });
        set_cull_mode(RHI_CULL_MODE_NONE);

        rhi_parameter_layout* material_layout = context->add_parameter_layout(
            "color pipeline",
            {
                {RHI_PARAMETER_TYPE_TEXTURE, 1}
        });

        set_parameter_layouts({
            {context->get_parameter_layout("violet mesh"),   RENDER_PIPELINE_PARAMETER_TYPE_MESH    },
            {material_layout,                                RENDER_PIPELINE_PARAMETER_TYPE_MATERIAL},
            {context->get_parameter_layout("violet camera"),
             RENDER_PIPELINE_PARAMETER_TYPE_CAMERA                                                  }
        });
    }

private:
    void render(rhi_render_command* command, render_data& data)
    {
        command->set_render_parameter(2, data.camera_parameter);
        for (render_mesh& mesh : data.meshes)
        {
            command->set_vertex_buffers(mesh.vertex_buffers.data(), mesh.vertex_buffers.size());
            command->set_index_buffer(mesh.index_buffer);
            command->set_render_parameter(0, mesh.transform);
            command->set_render_parameter(1, mesh.material);
            command->draw_indexed(0, 36, 0);
        }
    }
};

class physics_debug : public pei_debug_draw
{
public:
    physics_debug(
        render_graph* render_graph,
        render_subpass* debug_subpass,
        rhi_renderer* rhi,
        world& world)
        : m_position(2048),
          m_color(2048)
    {
        render_pipeline* pipeline = debug_subpass->add_pipeline<debug_pipeline>("debug");
        material_layout* layout = render_graph->add_material_layout("debug");
        layout->add_pipeline(pipeline);

        material* material = layout->add_material("debug");
        m_geometry = std::make_unique<geometry>(rhi);

        m_geometry->add_attribute("position", m_position, true);
        m_geometry->add_attribute("color", m_color, true);
        m_position.clear();
        m_color.clear();

        m_object = std::make_unique<actor>("physics debug", world);
        auto [transform_ptr, mesh_ptr] = m_object->add<transform, mesh>();

        mesh_ptr->set_geometry(m_geometry.get());
        mesh_ptr->add_submesh(0, 0, 0, 0, material);
    }

    void tick()
    {
        component_ptr<mesh> mesh_ptr = m_object->get<mesh>();
        std::memcpy(
            mesh_ptr->get_geometry()->get_vertex_buffer("position")->get_buffer(),
            m_position.data(),
            m_position.size() * sizeof(float3));
        std::memcpy(
            mesh_ptr->get_geometry()->get_vertex_buffer("color")->get_buffer(),
            m_color.data(),
            m_color.size() * sizeof(float3));
        mesh_ptr->set_submesh(0, 0, m_position.size(), 0, 0);

        m_position.clear();
        m_color.clear();
    }

    virtual void draw_line(const float3& start, const float3& end, const float3& color) override
    {
        m_position.push_back(start);
        m_position.push_back(end);
        m_color.push_back(color);
        m_color.push_back(color);
    }

private:
    std::unique_ptr<geometry> m_geometry;

    std::unique_ptr<actor> m_object;

    std::vector<float3> m_position;
    std::vector<float3> m_color;
};

class physics_demo : public engine_system
{
public:
    physics_demo() : engine_system("physics_demo"), m_depth_stencil(nullptr) {}

    virtual bool initialize(const dictionary& config)
    {
        auto& window = get_system<window_system>();
        window.on_resize().then(
            [this](std::uint32_t width, std::uint32_t height)
            {
                log::info("Window resize: {} {}", width, height);
                resize(width, height);
            });

        on_tick().then(
            [this](float delta)
            {
                tick(delta);
                get_system<graphics_system>().render(m_render_graph.get());
            });

        initialize_render();
        intiialize_physics();

        {
            m_cube1 = std::make_unique<actor>("cube 1", get_world());
            auto [mesh_ptr, transform_ptr, rigidbody_ptr] =
                m_cube1->add<mesh, transform, rigidbody>();
            mesh_ptr->set_geometry(m_geometry.get());
            mesh_ptr->add_submesh(0, 0, 0, 12, m_material);

            rigidbody_ptr->set_transform(transform_ptr->get_world_matrix());
            rigidbody_ptr->set_type(PEI_RIGIDBODY_TYPE_DYNAMIC);
            rigidbody_ptr->set_shape(m_collision_shape);
            rigidbody_ptr->set_mass(1.0f);
            m_physics_world->add(m_cube1.get());
        }

        {
            m_cube2 = std::make_unique<actor>("cube 2", get_world());
            auto [mesh_ptr, transform_ptr, rigidbody_ptr] =
                m_cube2->add<mesh, transform, rigidbody>();
            mesh_ptr->set_geometry(m_geometry.get());
            mesh_ptr->add_submesh(0, 0, 0, 12, m_material);

            transform_ptr->set_position(float3{2.0f, 0.0f, 0.0f});

            rigidbody_ptr->set_transform(transform_ptr->get_world_matrix());
            rigidbody_ptr->set_type(PEI_RIGIDBODY_TYPE_DYNAMIC);
            rigidbody_ptr->set_shape(m_collision_shape);
            rigidbody_ptr->set_mass(1.0f);

            joint* joint = rigidbody_ptr->add_joint();
            joint->set_target(m_cube1->get<rigidbody>());
            joint->set_linear({-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f});
            joint->set_spring_enable(0, true);
            joint->set_stiffness(0, 100.0f);
            joint->set_damping(0, 5.0f);

            m_physics_world->add(m_cube2.get());
        }

        {
            m_plane = std::make_unique<actor>("plane", get_world());
            auto [mesh_ptr, transform_ptr, rigidbody_ptr] =
                m_plane->add<mesh, transform, rigidbody>();
            mesh_ptr->set_geometry(m_geometry.get());
            mesh_ptr->add_submesh(0, 0, 0, 12, m_material);

            transform_ptr->set_position(0.0f, -3.0f, 0.0f);

            rigidbody_ptr->set_transform(transform_ptr->get_world_matrix());
            rigidbody_ptr->set_type(PEI_RIGIDBODY_TYPE_KINEMATIC);
            rigidbody_ptr->set_shape(m_collision_shape);
            rigidbody_ptr->set_mass(0.0f);

            joint* joint = rigidbody_ptr->add_joint();
            joint->set_target(m_cube2->get<rigidbody>());
            joint->set_linear({-5.0f, -5.0f, -5.0f}, {5.0f, 5.0f, 5.0f});
            joint->set_spring_enable(0, true);
            joint->set_stiffness(0, 100.0f);
            joint->set_damping(0, 5.0f);

            m_physics_world->add(m_plane.get());
        }

        return true;
    }

    virtual void shutdown()
    {
        m_cube1 = nullptr;
        m_cube2 = nullptr;
        m_plane = nullptr;
        m_camera = nullptr;

        m_render_graph = nullptr;
        m_geometry = nullptr;

        rhi_renderer* rhi = get_system<graphics_system>().get_context()->get_rhi();
        rhi->destroy_depth_stencil_buffer(m_depth_stencil);

        pei_plugin* pei = get_system<physics_system>().get_pei();
        pei->destroy_collision_shape(m_collision_shape);
    }

private:
    void initialize_render()
    {
        auto& graphics = get_system<graphics_system>();
        auto& window = get_system<window_system>();
        auto extent = window.get_extent();

        rhi_renderer* rhi = graphics.get_context()->get_rhi();

        m_render_graph = std::make_unique<render_graph>(graphics.get_context());

        render_pass* main = m_render_graph->add_render_pass("main");

        render_attachment* output_attachment = main->add_attachment("output");
        output_attachment->set_format(rhi->get_back_buffer()->get_format());
        output_attachment->set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
        output_attachment->set_final_state(RHI_RESOURCE_STATE_PRESENT);
        output_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
        output_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_STORE);

        render_attachment* depth_stencil_attachment = main->add_attachment("depth stencil");
        depth_stencil_attachment->set_format(RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT);
        depth_stencil_attachment->set_initial_state(RHI_RESOURCE_STATE_UNDEFINED);
        depth_stencil_attachment->set_final_state(RHI_RESOURCE_STATE_DEPTH_STENCIL);
        depth_stencil_attachment->set_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
        depth_stencil_attachment->set_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);
        depth_stencil_attachment->set_stencil_load_op(RHI_ATTACHMENT_LOAD_OP_CLEAR);
        depth_stencil_attachment->set_stencil_store_op(RHI_ATTACHMENT_STORE_OP_DONT_CARE);

        render_subpass* color_pass = main->add_subpass("color");
        color_pass->add_reference(
            output_attachment,
            RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
            RHI_RESOURCE_STATE_RENDER_TARGET);
        color_pass->add_reference(
            depth_stencil_attachment,
            RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,
            RHI_RESOURCE_STATE_DEPTH_STENCIL);

        render_pipeline* pipeline = color_pass->add_pipeline<color_pipeline>("color");

        main->add_dependency(
            nullptr,
            RHI_PIPELINE_STAGE_FLAG_COLOR_OUTPUT | RHI_PIPELINE_STAGE_FLAG_EARLY_DEPTH_STENCIL,
            0,
            color_pass,
            RHI_PIPELINE_STAGE_FLAG_COLOR_OUTPUT | RHI_PIPELINE_STAGE_FLAG_EARLY_DEPTH_STENCIL,
            RHI_ACCESS_FLAG_COLOR_WRITE | RHI_ACCESS_FLAG_DEPTH_STENCIL_WRITE);

        m_physics_debug =
            std::make_unique<physics_debug>(m_render_graph.get(), color_pass, rhi, get_world());

        m_render_graph->compile();

        m_geometry = std::make_unique<geometry>(rhi);
        std::vector<float3> position = {
            {-0.5f, -0.5f, 0.5f },
            {0.5f,  -0.5f, 0.5f },
            {0.5f,  0.5f,  0.5f },
            {-0.5f, 0.5f,  0.5f },
            {-0.5f, -0.5f, -0.5f},
            {0.5f,  -0.5f, -0.5f},
            {0.5f,  0.5f,  -0.5f},
            {-0.5f, 0.5f,  -0.5 }
        };
        m_geometry->add_attribute("position", position);
        std::vector<float3> color = {
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 1.0f},
            {0.0f, 0.0f, 1.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
            {0.0f, 1.0f, 1.0f},
            {0.0f, 0.0f, 1.0f}
        };
        m_geometry->add_attribute("color", color);
        std::vector<std::uint32_t> indices = {0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4, 7, 3, 0, 0, 4, 7,
                                              1, 5, 6, 6, 2, 1, 3, 2, 6, 6, 7, 3, 0, 1, 5, 5, 4, 0};
        m_geometry->set_indices(indices);

        material_layout* material_layout = m_render_graph->add_material_layout("text material");
        material_layout->add_pipeline(pipeline);
        material_layout->add_field(
            "texture",
            {.pipeline_index = 0, .field_index = 0, .size = 1, .offset = 0});

        m_material = material_layout->add_material("test");

        m_camera = std::make_unique<actor>("main camera", get_world());
        auto [camera_ptr, transform_ptr, orbit_control_ptr] =
            m_camera->add<camera, transform, orbit_control>();
        camera_ptr->set_render_pass(main);
        camera_ptr->set_attachment(0, rhi->get_back_buffer(), true);
        camera_ptr->resize(extent.width, extent.height);

        transform_ptr->set_position(0.0f, 0.0f, -10.0f);

        orbit_control_ptr->r = 10.0f;

        resize(extent.width, extent.height);
    }

    void intiialize_physics()
    {
        auto& physics = get_system<physics_system>();

        m_physics_world = std::make_unique<physics_world>(
            float3{0.0f, -9.8f, 0.0f},
            m_physics_debug.get(),
            physics.get_pei());

        pei_collision_shape_desc shape_desc = {};
        shape_desc.type = PEI_COLLISION_SHAPE_TYPE_BOX;
        shape_desc.box.height = 1.0f;
        shape_desc.box.width = 1.0f;
        shape_desc.box.length = 1.0f;
        m_collision_shape = physics.get_pei()->create_collision_shape(shape_desc);
    }

    void tick(float delta)
    {
        auto& physics = get_system<physics_system>();
        physics.simulation(m_physics_world.get());

        m_physics_debug->draw_line({0.0f, 0.0f, 0.0f}, {0.0f, 10.0f, 0.0f}, {1.0f, 0.0f, 0.0f});
        m_physics_debug->tick();
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

        auto transform_ptr = m_cube1->get<transform>();
        transform_ptr->set_rotation(
            quaternion_simd::rotation_axis(simd::set(1.0f, 0.0f, 0.0f, 0.0f), m_rotate));

        m_rotate += delta * 2.0f;
    }

    void resize(std::uint32_t width, std::uint32_t height)
    {
        rhi_renderer* rhi = get_system<graphics_system>().get_context()->get_rhi();
        rhi->destroy_depth_stencil_buffer(m_depth_stencil);

        rhi_depth_stencil_buffer_desc depth_stencil_buffer_desc = {};
        depth_stencil_buffer_desc.width = width;
        depth_stencil_buffer_desc.height = height;
        depth_stencil_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
        depth_stencil_buffer_desc.format = RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT;
        m_depth_stencil = rhi->create_depth_stencil_buffer(depth_stencil_buffer_desc);

        if (m_camera)
        {
            auto camera_ptr = m_camera->get<camera>();
            camera_ptr->resize(width, height);
            camera_ptr->set_attachment(1, m_depth_stencil);
        }
    }

    std::unique_ptr<actor> m_camera;
    std::unique_ptr<actor> m_cube1;
    std::unique_ptr<actor> m_cube2;
    std::unique_ptr<actor> m_plane;

    std::unique_ptr<geometry> m_geometry;
    material* m_material;

    std::unique_ptr<render_graph> m_render_graph;
    rhi_resource* m_depth_stencil;

    std::unique_ptr<physics_world> m_physics_world;
    pei_collision_shape* m_collision_shape;
    std::unique_ptr<physics_debug> m_physics_debug;

    float m_rotate = 0.0f;
};
} // namespace violet::sample

int main()
{
    using namespace violet;

    engine engine;

    engine.initialize("physics/config");
    engine.install<window_system>();
    engine.install<scene_system>();
    engine.install<graphics_system>();
    engine.install<physics_system>();
    engine.install<control_system>();
    engine.install<sample::physics_demo>();

    engine.get_system<window_system>().on_destroy().then(
        [&engine]()
        {
            log::info("Close window");
            engine.exit();
        });

    engine.run();

    return 0;
}
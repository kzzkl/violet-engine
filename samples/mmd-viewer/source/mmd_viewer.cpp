#include "mmd_viewer.hpp"
#include "components/camera.hpp"
#include "components/light.hpp"
#include "components/mesh.hpp"
#include "components/mmd_skeleton.hpp"
#include "components/orbit_control.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_system.hpp"
#include "mmd_animation.hpp"
#include "physics/physics_system.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class physics_debug : public pei_debug_draw
{
public:
    physics_debug(render_graph* render_graph, renderer* renderer, world& world)
        : m_position(1024 * 64),
          m_color(1024 * 64)
    {
        material* material = render_graph->add_material("debug", "debug material");
        m_geometry = std::make_unique<geometry>(renderer);

        m_geometry->add_attribute(
            "position",
            m_position,
            RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_HOST_VISIBLE);
        m_geometry->add_attribute(
            "color",
            m_color,
            RHI_BUFFER_FLAG_VERTEX | RHI_BUFFER_FLAG_HOST_VISIBLE);
        m_position.clear();
        m_color.clear();

        m_object = std::make_unique<actor>("physics debug", world);
        auto [debug_transform, debug_mesh] = m_object->add<transform, mesh>();

        debug_mesh->set_geometry(m_geometry.get());
        debug_mesh->add_submesh(0, 0, 0, 0, material);
    }

    void tick()
    {
        mesh* debug_mesh = m_object->get<mesh>().get();
        std::memcpy(
            debug_mesh->get_geometry()->get_vertex_buffer("position")->get_buffer(),
            m_position.data(),
            m_position.size() * sizeof(float3));
        std::memcpy(
            debug_mesh->get_geometry()->get_vertex_buffer("color")->get_buffer(),
            m_color.data(),
            m_color.size() * sizeof(float3));
        debug_mesh->set_submesh(0, 0, m_position.size(), 0, 0);

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

class mmd_skeleton_info : public component_info_default<mmd_skeleton>
{
public:
    mmd_skeleton_info(renderer* renderer) : m_renderer(renderer) {}

    virtual void construct(actor* owner, void* target) override
    {
        new (target) mmd_skeleton(m_renderer);
    }

private:
    renderer* m_renderer;
};

mmd_viewer::mmd_viewer() : engine_system("mmd viewer"), m_depth_stencil(nullptr)
{
}

mmd_viewer::~mmd_viewer()
{
}

bool mmd_viewer::initialize(const dictionary& config)
{
    on_tick().then(
        [this](float delta)
        {
            tick(delta);
        });

    get_system<window_system>().on_resize().then(
        [this](std::uint32_t width, std::uint32_t height)
        {
            resize(width, height);
        });

    initialize_render();

    renderer* renderer = get_system<graphics_system>().get_renderer();
    get_world().register_component<mmd_skeleton, mmd_skeleton_info>(renderer);

    m_loader = std::make_unique<mmd_loader>(
        m_render_graph.get(),
        renderer,
        get_system<physics_system>().get_context());
    m_model = m_loader->load(config["model"], config["motion"], get_world());
    if (!m_model)
        return false;

    m_physics_debug = std::make_unique<physics_debug>(m_render_graph.get(), renderer, get_world());
    m_physics_world = std::make_unique<physics_world>(
        float3{0.0f, -9.8f, 0.0f},
        nullptr, // m_physics_debug.get(),
        get_system<physics_system>().get_context());

    get_system<mmd_animation>().evaluate(0);
    get_system<mmd_animation>().update(false);
    get_system<mmd_animation>().update(true);

    for (std::size_t i = 0; i < m_model->bones.size(); ++i)
    {
        auto bone_rigidbody = m_model->bones[i]->get<rigidbody>();
        if (bone_rigidbody)
            m_physics_world->add(m_model->bones[i].get());
    }

    m_light = std::make_unique<actor>("main light", get_world());
    auto [light_transform, main_light] = m_light->add<transform, light>();
    main_light->color = {1.0f, 1.0f, 1.0f};

    return true;
}

void mmd_viewer::shutdown()
{
    m_loader = nullptr;
}

void mmd_viewer::initialize_render()
{
    auto& graphics = get_system<graphics_system>();

    m_render_graph = std::make_unique<mmd_render_graph>(graphics.get_renderer());
    m_render_graph->compile();

    auto extent = get_system<window_system>().get_extent();
    resize(extent.width, extent.height);
}

void mmd_viewer::tick(float delta)
{
    compute_pipeline* skinning_pipeline = m_render_graph->get_compute_pipeline("skinning pipeline");
    view<mmd_skeleton, mesh, transform> view(get_world());

    static float total_delta = 0.0f;
    total_delta += delta;
    get_system<mmd_animation>().evaluate(total_delta * 30.0f);
    get_system<mmd_animation>().update(false);
    get_system<physics_system>().simulation(m_physics_world.get(), true);
    get_system<mmd_animation>().update(true);

    m_physics_debug->tick();

    view.each(
        [&](mmd_skeleton& model_skeleton, mesh& model_mesh, transform& model_transform)
        {
            float4x4_simd world_to_local =
                matrix_simd::inverse_transform(simd::load(model_transform.get_world_matrix()));

            for (std::size_t i = 0; i < model_skeleton.bones.size(); ++i)
            {
                auto bone_transform = model_skeleton.bones[i].transform;
                model_skeleton.local_matrices[i] = bone_transform->get_local_matrix();

                float4x4_simd final_transform = simd::load(bone_transform->get_world_matrix());
                final_transform = matrix_simd::mul(final_transform, world_to_local);

                float4x4_simd initial_inverse = simd::load(model_skeleton.bones[i].initial_inverse);
                final_transform = matrix_simd::mul(initial_inverse, final_transform);

                mmd_skinning_bone data;
                simd::store(matrix_simd::transpose(final_transform), data.offset);
                simd::store(quaternion_simd::rotation_matrix(final_transform), data.quaternion);

                model_skeleton.get_skeleton_parameter()->set_uniform(
                    0,
                    &data,
                    sizeof(mmd_skinning_bone),
                    i * sizeof(mmd_skinning_bone));
            }

            skinning_pipeline->add_dispatch(
                (model_mesh.get_geometry()->get_vertex_count() + 255) / 256,
                1,
                1,
                {model_skeleton.get_skeleton_parameter(), model_skeleton.get_skinning_parameter()});
        });

    get_system<graphics_system>().render(m_render_graph.get());
}

void mmd_viewer::resize(std::uint32_t width, std::uint32_t height)
{
    renderer* renderer = get_system<graphics_system>().get_renderer();

    rhi_image_desc depth_stencil_desc = {};
    depth_stencil_desc.width = width;
    depth_stencil_desc.height = height;
    depth_stencil_desc.samples = RHI_SAMPLE_COUNT_1;
    depth_stencil_desc.format = RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    depth_stencil_desc.flags = RHI_IMAGE_FLAG_DEPTH_STENCIL;
    m_depth_stencil = renderer->create_image(depth_stencil_desc);

    if (m_camera)
    {
        auto main_camera = m_camera->get<camera>();
        main_camera->resize(width, height);
        main_camera->set_attachment(1, m_depth_stencil.get());
    }
    else
    {
        m_camera = std::make_unique<actor>("main camera", get_world());
        auto [main_camera, main_camera_transform, main_camera_controller] =
            m_camera->add<camera, transform, orbit_control>();
        main_camera->set_render_pass(m_render_graph->get_render_pass("main"));
        main_camera->set_attachment(0, renderer->get_back_buffer(), true);
        main_camera->set_attachment(1, m_depth_stencil.get());
        main_camera->resize(width, height);
        main_camera_transform->set_position(0.0f, 2.0f, -5.0f);
        main_camera_controller->r = 50.0f;
        main_camera_controller->target = {0.0f, 15.0f, 0.0f};
    }
}
} // namespace violet::sample
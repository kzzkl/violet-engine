#include "mmd_viewer.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/rigidbody.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_system.hpp"
#include "mmd_skeleton.hpp"
#include "physics/physics_system.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{
class physics_debug : public pei_debug_draw
{
public:
    physics_debug(render_graph* render_graph, rhi_renderer* rhi, world& world)
        : m_position(1024 * 64),
          m_color(1024 * 64)
    {
        material* material = render_graph->add_material("debug", "debug material");
        m_geometry = std::make_unique<geometry>(rhi);

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
    mmd_skeleton_info(
        rhi_renderer* rhi,
        rhi_parameter_layout* skeleton_layout,
        rhi_parameter_layout* skinning_layout)
        : m_rhi(rhi),
          m_skeleton_layout(skeleton_layout),
          m_skinning_layout(skinning_layout)
    {
    }

    virtual void construct(actor* owner, void* target) override
    {
        new (target) mmd_skeleton(m_rhi, m_skeleton_layout, m_skinning_layout);
    }

private:
    rhi_renderer* m_rhi;
    rhi_parameter_layout* m_skeleton_layout;
    rhi_parameter_layout* m_skinning_layout;
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

    graphics_context* graphics_context = get_system<graphics_system>().get_context();
    get_world().register_component<mmd_skeleton, mmd_skeleton_info>(
        graphics_context->get_rhi(),
        graphics_context->get_parameter_layout("mmd skeleton"),
        graphics_context->get_parameter_layout("mmd skinning"));
    get_world().register_component<mmd_bone>();

    m_loader = std::make_unique<mmd_loader>(
        m_render_graph.get(),
        get_system<graphics_system>().get_context()->get_rhi(),
        get_system<physics_system>().get_pei());
    m_model = m_loader->load(config["model"], get_world());

    m_physics_debug = std::make_unique<physics_debug>(
        m_render_graph.get(),
        get_system<graphics_system>().get_context()->get_rhi(),
        get_world());
    m_physics_world = std::make_unique<physics_world>(
        float3{0.0f, -9.8f, 0.0f},
        nullptr,//m_physics_debug.get(),
        get_system<physics_system>().get_pei());

    for (std::size_t i = 0; i < m_model->bones.size(); ++i)
    {
        auto bone_transform = m_model->bones[i]->get<transform>();
        auto world = bone_transform->get_world_matrix();

        auto bone_rigidbody = m_model->bones[i]->get<rigidbody>();
        if (bone_rigidbody)
            m_physics_world->add(m_model->bones[i].get());
    }

    return true;
}

void mmd_viewer::shutdown()
{
    m_loader = nullptr;
}

void mmd_viewer::initialize_render()
{
    auto& graphics = get_system<graphics_system>();
    rhi_renderer* rhi = graphics.get_context()->get_rhi();

    m_render_graph = std::make_unique<mmd_render_graph>(graphics.get_context());
    m_render_graph->compile();

    auto extent = get_system<window_system>().get_extent();
    resize(extent.width, extent.height);
}

void mmd_viewer::tick(float delta)
{
    if (get_system<window_system>().get_keyboard().key(KEYBOARD_KEY_1).release())
    {
        // auto bone = m_model->bones[15]->get<transform>();
        // bone->set_position(vector::add(bone->get_position(), float3{1.0f, 0.0f, 0.0f}));

        get_system<physics_system>().simulation(m_physics_world.get());
        m_physics_debug->tick();
    }

    compute_pipeline* skinning_pipeline = m_render_graph->get_compute_pipeline("skinning pipeline");
    view<mmd_skeleton, mesh, transform> view(get_world());
    /*view.each(
        [this, skinning_pipeline](
            mmd_skeleton& model_skeleton,
            mesh& model_mesh,
            transform& model_transform)
        {
            float4_simd scale;
            float4_simd rotation;
            float4_simd position;
            for (std::size_t i = 0; i < model_skeleton.bones.size(); ++i)
            {
                auto bone_transform = model_skeleton.bones[i]->get<transform>();
                matrix_simd::decompose(
                    simd::load(model_skeleton.local_matrices[i]),
                    scale,
                    rotation,
                    position);
                bone_transform->set_scale(scale);
                bone_transform->set_rotation(rotation);
                bone_transform->set_position(position);
            }
        });*/

    // physics
    // animation
    get_system<physics_system>().simulation(m_physics_world.get());
    m_physics_debug->tick();

    view.each(
        [&](mmd_skeleton& model_skeleton, mesh& model_mesh, transform& model_transform)
        {
            float4x4_simd world_to_local =
                matrix_simd::inverse_transform(simd::load(model_transform.get_world_matrix()));

            for (std::size_t i = 0; i < model_skeleton.bones.size(); ++i)
            {
                auto bone_transform = model_skeleton.bones[i]->get<transform>();
                model_skeleton.local_matrices[i] = bone_transform->get_local_matrix();

                float4x4_simd final_transform = simd::load(bone_transform->get_world_matrix());
                final_transform = matrix_simd::mul(final_transform, world_to_local);

                float4x4_simd initial_inverse =
                    simd::load(model_skeleton.bones[i]->get<mmd_bone>()->initial_inverse);
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

    // m_camera->get_component<orbit_control>()->phi += delta;
    // m_camera->get_component<orbit_control>()->dirty = true;
}

void mmd_viewer::resize(std::uint32_t width, std::uint32_t height)
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
        auto main_camera = m_camera->get<camera>();
        main_camera->resize(width, height);
        main_camera->set_attachment(1, m_depth_stencil);
    }
    else
    {
        m_camera = std::make_unique<actor>("main camera", get_world());
        auto [main_camera, main_camera_transform, main_camera_controller] =
            m_camera->add<camera, transform, orbit_control>();
        main_camera->set_render_pass(m_render_graph->get_render_pass("main"));
        main_camera->set_attachment(0, rhi->get_back_buffer(), true);
        main_camera->set_attachment(1, m_depth_stencil);
        main_camera->resize(width, height);
        main_camera_transform->set_position(0.0f, 2.0f, -5.0f);
        main_camera_controller->r = 50.0f;
        main_camera_controller->target = {0.0f, 15.0f, 0.0f};
    }
}
} // namespace violet::sample
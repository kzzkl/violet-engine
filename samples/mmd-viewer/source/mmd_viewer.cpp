#include "mmd_viewer.hpp"
#include "mmd_animation.hpp"
#include "scene.hpp"
#include "transform.hpp"

namespace ash::sample::mmd
{
bool mmd_viewer::initialize(const dictionary& config)
{
    auto& world = system<ash::ecs::world>();
    world.register_component<mmd_node>();
    world.register_component<mmd_skeleton>();

    m_skeleton_view = world.make_view<mmd_skeleton>();

    m_loader = std::make_unique<mmd_loader>(
        world,
        system<core::relation>(),
        system<graphics::graphics>(),
        system<scene::scene>(),
        system<physics::physics>());
    m_loader->initialize();

    initialize_pass();

    return true;
}

ash::ecs::entity mmd_viewer::load_mmd(
    std::string_view name,
    std::string_view pmx,
    std::string_view vmd)
{
    ecs::entity entity = system<ecs::world>().create();
    mmd_resource resource;
    if (m_loader->load(entity, resource, pmx, vmd, m_render_pass.get()))
    {
        m_resources[name.data()] = std::move(resource);
        return entity;
    }
    else
    {
        system<ecs::world>().release(entity);
        return ecs::INVALID_ENTITY;
    }
}

void mmd_viewer::update()
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& animation = system<mmd_animation>();

    m_skeleton_view->each([&](ecs::entity entity, mmd_skeleton& skeleton) { reset(entity); });

    static float delta = 0.0f;
    delta += system<core::timer>().frame_delta();
    animation.evaluate(delta * 30.0f);
    animation.update(false);
    m_skeleton_view->each([&](mmd_skeleton& skeleton) {
        for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
        {
            auto& transform = world.component<scene::transform>(skeleton.nodes[i]);
            math::matrix_plain::decompose(
                skeleton.local[i],
                transform.scaling,
                transform.rotation,
                transform.position);
            transform.dirty = true;
        }
    });
    scene.sync_local();

    system<physics::physics>().simulation();

    animation.update(true);

    m_skeleton_view->each([&](mmd_skeleton& skeleton) {
        for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
        {
            auto& transform = world.component<scene::transform>(skeleton.nodes[i]);

            math::float4x4_simd to_world = math::simd::load(transform.world_matrix);
            math::float4x4_simd initial =
                math::simd::load(world.component<mmd_node>(skeleton.nodes[i]).initial_inverse);

            math::float4x4_simd final_transform = math::matrix_simd::mul(initial, to_world);
            math::simd::store(final_transform, skeleton.world[i]);
        }

        skeleton.parameter->set(0, skeleton.world.data(), skeleton.world.size());
    });
}

void mmd_viewer::reset(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& skeleton = world.component<mmd_skeleton>(entity);

    for (auto& node_entity : skeleton.nodes)
    {
        auto& node = world.component<mmd_node>(node_entity);
        auto& node_transform = world.component<scene::transform>(node_entity);
        node_transform.dirty = true;

        node_transform.position = node.initial_position;
        node_transform.rotation = node.initial_rotation;
        node_transform.scaling = node.initial_scale;

        node.inherit_translate = {0.0f, 0.0f, 0.0f};
        node.inherit_rotate = {0.0f, 0.0f, 0.0f, 1.0f};
    }
}

void mmd_viewer::initialize_pose(ecs::entity entity)
{
    auto& world = system<ecs::world>();
    auto& scene = system<scene::scene>();
    auto& animation = system<mmd_animation>();

    reset(entity);

    animation.evaluate(0.0f);
    animation.update(false);
    system<physics::physics>().simulation();
    animation.update(true);

    auto& skeleton = world.component<mmd_skeleton>(entity);
    for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
    {
        auto& transform = world.component<scene::transform>(skeleton.nodes[i]);
        math::matrix_plain::decompose(
            skeleton.local[i],
            transform.scaling,
            transform.rotation,
            transform.position);
        transform.dirty = true;

        if (world.has_component<physics::rigidbody>(skeleton.nodes[i]))
        {
            auto& rigidbody = world.component<physics::rigidbody>(skeleton.nodes[i]);
            rigidbody.interface->angular_velocity(math::float3{0.0f, 0.0f, 0.0f});
            rigidbody.interface->linear_velocity(math::float3{0.0f, 0.0f, 0.0f});
            rigidbody.interface->clear_forces();
        }
    }
    scene.sync_local(entity);

    for (std::size_t i = 0; i < skeleton.nodes.size(); ++i)
    {
        auto& transform = world.component<scene::transform>(skeleton.nodes[i]);

        math::float4x4_simd to_world = math::simd::load(transform.world_matrix);
        math::float4x4_simd initial =
            math::simd::load(world.component<mmd_node>(skeleton.nodes[i]).initial_inverse);

        math::float4x4_simd final_transform = math::matrix_simd::mul(initial, to_world);
        math::simd::store(final_transform, skeleton.world[i]);
    }
    skeleton.parameter->set(0, skeleton.world.data(), skeleton.world.size());
}

void mmd_viewer::initialize_pass()
{
    auto& graphics = system<graphics::graphics>();

    graphics::pipeline_parameter_layout_info mmd_material;
    mmd_material.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4,  1}, // diffuse
        {graphics::pipeline_parameter_type::FLOAT3,  1}, // specular
        {graphics::pipeline_parameter_type::FLOAT,   1}, // specular_strength
        {graphics::pipeline_parameter_type::UINT,    1}, // toon_mode
        {graphics::pipeline_parameter_type::UINT,    1}, // spa_mode
        {graphics::pipeline_parameter_type::TEXTURE, 1}, // tex
        {graphics::pipeline_parameter_type::TEXTURE, 1}, // toon
        {graphics::pipeline_parameter_type::TEXTURE, 1}  // spa
    };
    graphics.make_render_parameter_layout("mmd_material", mmd_material);

    graphics::pipeline_parameter_layout_info mmd_skeleton;
    mmd_skeleton.parameters = {
        {graphics::pipeline_parameter_type::FLOAT4x4_ARRAY, 512}, // offset
    };
    graphics.make_render_parameter_layout("mmd_skeleton", mmd_skeleton);

    // Pass.
    graphics::pipeline_info color_pass_info = {};
    color_pass_info.vertex_shader = "resource/shader/glsl/vert.spv";
    color_pass_info.pixel_shader = "resource/shader/glsl/frag.spv";
    color_pass_info.vertex_layout.attributes = {
        graphics::vertex_attribute_type::FLOAT3, // position
        graphics::vertex_attribute_type::FLOAT3, // normal
        graphics::vertex_attribute_type::FLOAT2, // uv
        graphics::vertex_attribute_type::UINT4,  // bone
        graphics::vertex_attribute_type::FLOAT3, // bone weight
    };
    color_pass_info.input = {};
    color_pass_info.output = {0};
    color_pass_info.depth = 1;
    color_pass_info.output_depth = true;
    color_pass_info.primitive_topology = graphics::primitive_topology_type::TRIANGLE_LIST;
    color_pass_info.pipeline_layout_info
        .parameters = {"ash_object", "mmd_material", "mmd_skeleton", "ash_pass"};

    // Render target.
    graphics::render_target_info render_target = {};
    render_target.format = graphics::resource_format::R8G8B8A8_UNORM;
    render_target.load_op = graphics::render_target_desc::load_op_type::CLEAR;
    render_target.store_op = graphics::render_target_desc::store_op_type::STORE;
    render_target.stencil_load_op = graphics::render_target_desc::load_op_type::DONT_CARE;
    render_target.stencil_store_op = graphics::render_target_desc::store_op_type::DONT_CARE;
    render_target.samples = 1;
    render_target.initial_state = graphics::resource_state::UNDEFINED;
    render_target.final_state = graphics::resource_state::PRESENT;

    graphics::render_target_info depth_stencil = {};
    depth_stencil.format = graphics::resource_format::D32_FLOAT_S8_UINT;
    depth_stencil.load_op = graphics::render_target_desc::load_op_type::CLEAR;
    depth_stencil.store_op = graphics::render_target_desc::store_op_type::DONT_CARE;
    depth_stencil.stencil_load_op = graphics::render_target_desc::load_op_type::DONT_CARE;
    depth_stencil.stencil_store_op = graphics::render_target_desc::store_op_type::DONT_CARE;
    depth_stencil.samples = 1;
    depth_stencil.initial_state = graphics::resource_state::UNDEFINED;
    depth_stencil.final_state = graphics::resource_state::DEPTH_STENCIL;

    graphics::render_pass_info mmd_pass_info;
    mmd_pass_info.render_targets.push_back(render_target);
    mmd_pass_info.render_targets.push_back(depth_stencil);
    mmd_pass_info.subpasses.push_back(color_pass_info);

    m_render_pass = graphics.make_render_pass<mmd_pass>(mmd_pass_info);
    m_render_pass->initialize_render_target_set(graphics);
}
} // namespace ash::sample::mmd
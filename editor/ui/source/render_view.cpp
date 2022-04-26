#include "render_view.hpp"
#include "relation.hpp"
#include "scene.hpp"
#include "ui.hpp"
#include "window.hpp"
#include "world.hpp"

namespace ash::editor
{
render_view::render_view(core::context* context) : editor_view(context)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    m_scene_camera = world.create("scene_camera");
    world.add<core::link, graphics::camera, scene::transform>(m_scene_camera);

    auto& transform = world.component<scene::transform>(m_scene_camera);
    transform.position = {0.0f, 0.0f, -50.0f};
    transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    transform.scaling = {1.0f, 1.0f, 1.0f};

    auto& camera = world.component<graphics::camera>(m_scene_camera);
    camera.parameter = graphics.make_render_parameter("ash_pass");

    relation.link(m_scene_camera, scene.root());
}

void render_view::draw(editor_data& data)
{
    auto& ui = system<ui::ui>();
    auto& graphics = system<graphics::graphics>();
    auto& scene = system<scene::scene>();

    ui.style(ui::ui_style::WINDOW_PADDING, 0.0f, 0.0f);
    if (ui.window_ex("Render"))
        update_camera();

    auto active = ui.any_item_active();
    if (m_resize_flag != active)
    {
        if (m_resize_flag)
        {
            auto [width, height] = ui.window_size();
            if (width != m_target_width || height != m_target_height)
            {
                m_target_width = width;
                m_target_height = height;
                resize_target();
            }
        }
        m_resize_flag = active;
    }

    ui.texture(
        m_render_target.get(),
        static_cast<float>(m_target_width),
        static_cast<float>(m_target_height));

    scene.sync_local();
    graphics.render(m_scene_camera);
    ui.window_pop();
    ui.style_pop();
}

void render_view::update_camera()
{
    auto& mouse = system<window::window>().mouse();

    static bool flag = true;
    bool key_down = mouse.key(window::mouse_key::MIDDLE_BUTTON).down() ||
                    mouse.key(window::mouse_key::RIGHT_BUTTON).down() || mouse.whell() != 0;

    static math::float2 old_mouse = {0.0f, 0.0f};
    if (flag && key_down)
    {
        old_mouse[0] = static_cast<float>(mouse.x());
        old_mouse[1] = static_cast<float>(mouse.y());
        flag = false;
    }
    else if (!key_down)
    {
        flag = true;
    }

    if (!flag)
    {
        float delta = system<core::timer>().frame_delta();

        float relative_x = static_cast<float>(mouse.x()) - old_mouse[0];
        float relative_y = static_cast<float>(mouse.y()) - old_mouse[1];
        old_mouse[0] = static_cast<float>(mouse.x());
        old_mouse[1] = static_cast<float>(mouse.y());

        auto& transform = system<ecs::world>().component<scene::transform>(m_scene_camera);

        if (mouse.key(window::mouse_key::RIGHT_BUTTON).down())
        {
            math::float4x4_simd to_local =
                math::matrix_simd::inverse(math::simd::load(transform.world_matrix));

            math::float4_simd rotate_x = math::quaternion_simd::rotation_axis(
                math::simd::set(1.0f, 0.0f, 0.0f, 0.0f),
                relative_y * delta * m_camera_rotate_speed);
            math::float4_simd rotate_y = math::quaternion_simd::rotation_axis(
                to_local[1],
                relative_x * delta * m_camera_rotate_speed);

            math::float4_simd rotation = math::simd::load(transform.rotation);
            rotation = math::quaternion_simd::mul(rotation, rotate_y);
            rotation = math::quaternion_simd::mul(rotation, rotate_x);
            math::simd::store(rotation, transform.rotation);

            transform.dirty = true;
            system<scene::scene>().sync_local(m_scene_camera);
        }

        if (mouse.key(window::mouse_key::MIDDLE_BUTTON).down())
        {
            math::float4_simd up = math::simd::load(transform.world_matrix[1]);
            math::float4_simd right = math::simd::load(transform.world_matrix[0]);

            up = math::vector_simd::scale(up, relative_y * delta * m_camera_move_speed);
            right = math::vector_simd::scale(right, -relative_x * delta * m_camera_move_speed);

            math::float4_simd position = math::simd::load(transform.position);
            position = math::vector_simd::add(position, up);
            position = math::vector_simd::add(position, right);
            math::simd::store(position, transform.position);

            transform.dirty = true;
        }

        if (mouse.whell() != 0)
        {
            math::float4_simd forward = math::simd::load(transform.world_matrix[2]);
            forward =
                math::vector_simd::scale(forward, mouse.whell() * delta * m_camera_move_speed * 50);

            math::float4_simd position = math::simd::load(transform.position);
            position = math::vector_simd::add(position, forward);
            math::simd::store(position, transform.position);

            transform.dirty = true;
        }
    }
}

void render_view::resize_target()
{
    if (m_target_width == 0 || m_target_height == 0)
        return;

    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();

    m_render_target = graphics.make_render_target(m_target_width, m_target_height, 4);
    m_depth_stencil = graphics.make_depth_stencil(m_target_width, m_target_height, 4);

    auto& camera = world.component<graphics::camera>(m_scene_camera);
    camera.render_target = m_render_target.get();
    camera.depth_stencil = m_depth_stencil.get();
    camera.mask = graphics::visual::mask_type::DEBUG | graphics::visual::mask_type::GROUP_1;

    camera.set(
        math::to_radians(30.0f),
        static_cast<float>(m_target_width) / static_cast<float>(m_target_height),
        0.01f,
        1000.0f);
}

void render_view::draw_grid()
{
    auto& debug_draw = system<graphics::graphics>().debug();

    std::size_t width = 10;
    float half = static_cast<float>(width / 2);

    for (std::size_t i = 0; i < width; ++i)
    {
        debug_draw.draw_line(
            math::float3{-half, 0.0f, static_cast<float>(i)},
            math::float3{half, 0.0f, static_cast<float>(i)},
            math::float3{1.0f, 1.0f, 1.0f});
    }

    for (std::size_t j = 0; j < 10; ++j)
    {
        debug_draw.draw_line(
            math::float3{static_cast<float>(j), 0.0f, -half},
            math::float3{static_cast<float>(j), 0.0f, half},
            math::float3{1.0f, 1.0f, 1.0f});
    }
}
} // namespace ash::editor
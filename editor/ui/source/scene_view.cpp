#include "editor/scene_view.hpp"
#include "core/relation.hpp"
#include "ecs/world.hpp"
#include "graphics/graphics.hpp"
#include "scene/scene.hpp"
#include "ui/controls/image.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"

namespace ash::editor
{
scene_view::scene_view()
    : image(nullptr),
      m_camera_move_speed(5.0f),
      m_camera_rotate_speed(0.5f),
      m_mouse_flag(true),
      m_mouse_position{},
      m_width(0),
      m_height(0)
{
    auto& world = system<ecs::world>();
    auto& graphics = system<graphics::graphics>();
    auto& scene = system<scene::scene>();
    auto& relation = system<core::relation>();

    // Create camera.
    m_camera = world.create("scene camera");
    world.add<core::link, graphics::camera, scene::transform>(m_camera);

    auto& transform = world.component<scene::transform>(m_camera);
    transform.position = {0.0f, 0.0f, -50.0f};
    transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    transform.scaling = {1.0f, 1.0f, 1.0f};

    auto& camera = world.component<graphics::camera>(m_camera);
    camera.parameter = graphics.make_pipeline_parameter("ash_pass");

    relation.link(m_camera, scene.root());

    on_blur = [this]() { m_focused = false; };
    on_focus = [this]() {
        m_focused = true;
        auto& mouse = system<window::window>().mouse();
        m_mouse_position[0] = static_cast<float>(mouse.x());
        m_mouse_position[1] = static_cast<float>(mouse.y());
    };
}

void scene_view::tick()
{
    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();

    if (m_width == 0 || m_height == 0)
        return;

    if (m_focused)
        update_camera();

    graphics.render(m_camera);
}

void scene_view::on_extent_change(const ui::element_extent& element_extent)
{
    ui::image::on_extent_change(element_extent);

    auto view_extent = extent();
    if (static_cast<std::uint32_t>(view_extent.width) != m_width ||
        static_cast<std::uint32_t>(view_extent.height) != m_height)
    {
        m_width = static_cast<std::uint32_t>(view_extent.width);
        m_height = static_cast<std::uint32_t>(view_extent.height);
        resize_camera();

        texture(m_render_target_resolve.get());
    }
}

void scene_view::update_camera()
{
    auto& mouse = system<window::window>().mouse();

    bool key_down = mouse.key(window::MOUSE_KEY_MIDDLE).down() ||
                    mouse.key(window::MOUSE_KEY_RIGHT).down() || mouse.whell() != 0;

    if (m_mouse_flag && key_down)
    {
        m_mouse_position[0] = static_cast<float>(mouse.x());
        m_mouse_position[1] = static_cast<float>(mouse.y());
        m_mouse_flag = false;
    }
    else if (!key_down)
    {
        m_mouse_flag = true;
    }

    if (!m_mouse_flag)
    {
        float delta = system<core::timer>().frame_delta();

        float relative_x = static_cast<float>(mouse.x()) - m_mouse_position[0];
        float relative_y = static_cast<float>(mouse.y()) - m_mouse_position[1];
        m_mouse_position[0] = static_cast<float>(mouse.x());
        m_mouse_position[1] = static_cast<float>(mouse.y());

        auto& transform = system<ecs::world>().component<scene::transform>(m_camera);

        if (mouse.key(window::MOUSE_KEY_RIGHT).down())
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
            system<scene::scene>().sync_local(m_camera);
        }

        if (mouse.key(window::MOUSE_KEY_MIDDLE).down())
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

void scene_view::resize_camera()
{
    if (m_width == 0 || m_height == 0)
        return;

    auto& graphics = system<graphics::graphics>();
    auto& world = system<ecs::world>();

    auto& camera = world.component<graphics::camera>(m_camera);

    graphics::render_target_info render_target_info = {};
    render_target_info.width = m_width;
    render_target_info.height = m_height;
    render_target_info.format = graphics.back_buffer_format();
    render_target_info.samples = 4;
    m_render_target = graphics.make_render_target(render_target_info);
    camera.render_target = m_render_target.get();

    graphics::render_target_info render_target_resolve_info = {};
    render_target_resolve_info.width = m_width;
    render_target_resolve_info.height = m_height;
    render_target_resolve_info.format = graphics.back_buffer_format();
    render_target_resolve_info.samples = 1;
    m_render_target_resolve = graphics.make_render_target(render_target_resolve_info);
    camera.render_target_resolve = m_render_target_resolve.get();

    graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
    depth_stencil_buffer_info.width = m_width;
    depth_stencil_buffer_info.height = m_height;
    depth_stencil_buffer_info.format = graphics::resource_format::D24_UNORM_S8_UINT;
    depth_stencil_buffer_info.samples = 4;
    m_depth_stencil_buffer = graphics.make_depth_stencil_buffer(depth_stencil_buffer_info);
    camera.depth_stencil_buffer = m_depth_stencil_buffer.get();

    camera.mask = graphics::VISUAL_GROUP_DEBUG | graphics::VISUAL_GROUP_1;
    camera.set(
        math::to_radians(45.0f),
        static_cast<float>(m_width) / static_cast<float>(m_height),
        0.3f,
        1000.0f);
}
} // namespace ash::editor
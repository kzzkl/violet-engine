#include "render_view.hpp"

namespace ash::editor
{
render_view::render_view(
    graphics::graphics& graphics,
    ecs::world& world,
    core::relation& relation,
    scene::scene& scene)
    : m_graphics(graphics),
      m_world(world),
      m_relation(relation),
      m_scene(scene)
{
    m_scene_camera = world.create("scene_camera");
    world.add<core::link, graphics::camera, scene::transform>(m_scene_camera);

    auto& transform = world.component<scene::transform>(m_scene_camera);
    transform.position = {0.0f, 11.0f, -60.0f};
    transform.rotation = {0.0f, 0.0f, 0.0f, 1.0f};
    transform.scaling = {1.0f, 1.0f, 1.0f};

    auto& camera = world.component<graphics::camera>(m_scene_camera);
    camera.parameter = graphics.make_render_parameter("ash_pass");

    relation.link(m_scene_camera, scene.root());
}

void render_view::draw(ui::ui& ui, editor_data& data)
{
    ui.style(ui::ui_style::WINDOW_PADDING, 0.0f, 0.0f);
    ui.window("render");

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

    m_graphics.render(m_scene_camera);
    ui.window_pop();
    ui.style_pop();
}

void render_view::resize_target()
{
    log::debug("resize: {} {}", m_target_width, m_target_height);
    if (m_target_width == 0 || m_target_height == 0)
        return;

    m_render_target = m_graphics.make_render_target(m_target_width, m_target_height, 4);
    m_depth_stencil = m_graphics.make_depth_stencil(m_target_width, m_target_height, 4);
    // m_render_target = m_graphics.make_render_target(512, 512, 4);
    // m_depth_stencil = m_graphics.make_depth_stencil(512, 512, 4);

    auto& camera = m_world.component<graphics::camera>(m_scene_camera);
    camera.render_target = m_render_target.get();
    camera.depth_stencil = m_depth_stencil.get();
    camera.mask = 1;

    camera.set(
        math::to_radians(30.0f),
        static_cast<float>(m_target_width) / static_cast<float>(m_target_height),
        0.01f,
        1000.0f);
}
} // namespace ash::editor
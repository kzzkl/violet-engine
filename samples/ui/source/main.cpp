#include "assert.hpp"
#include "core/application.hpp"
#include "core/relation.hpp"
#include "graphics/graphics.hpp"
#include "scene/scene.hpp"
#include "ui/controls/label.hpp"
#include "ui/controls/panel.hpp"
#include "ui/controls/tree.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"

namespace ash::sample
{
class test_system : public core::system_base
{
public:
    test_system() : core::system_base("test") {}

    virtual bool initialize(const dictionary& config) override
    {
        initialize_task();
        initialize_ui();
        initialize_camera();

        system<core::event>().subscribe<window::event_window_resize>(
            "sample_module",
            [this](std::uint32_t width, std::uint32_t height) { resize_camera(width, height); });

        return true;
    }

private:
    void initialize_task()
    {
        auto& task = system<task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });
        auto window_task = task.schedule(
            "window tick",
            [this]() { system<window::window>().tick(); },
            task::task_type::MAIN_THREAD);
        auto render_task = task.schedule("render", [this]() {
            auto& graphics = system<graphics::graphics>();
            graphics.render(m_camera);
            graphics.present();
        });

        window_task->add_dependency(*task.find("root"));
        update_task->add_dependency(*window_task);
        render_task->add_dependency(*update_task);
    }

    void initialize_ui()
    {
        auto& ui = system<ui::ui>();
        auto& relation = system<core::relation>();
        auto& world = system<ecs::world>();

        ui.root()->flex_direction(ui::LAYOUT_FLEX_DIRECTION_COLUMN);

        m_panel = std::make_unique<ui::panel>(ui::COLOR_AQUA);
        m_panel->resize(300.0f, 300.0f);
        m_panel->link(ui.root());

        m_text = std::make_unique<ui::label>("hello world! qap", ui.font(), 0xFF00FF00);
        m_text->resize(100.0f, 20.0f);
        m_text->link(ui.root());

        m_tree = std::make_unique<ui::tree>();
        m_tree->resize(200.0f, 0.0f, false, true);
        m_tree->link(ui.root());

        m_tree_node = std::make_unique<ui::tree_node>("node 1", ui.font());
        m_tree_node->link(m_tree.get());

        m_tree_panel = std::make_unique<ui::panel>(ui::COLOR_GREEN);
        m_tree_panel->resize(50.0f, 50.0f);
        m_tree_panel->link(m_tree_node->container());

        m_tree_node_2 = std::make_unique<ui::tree_node>(
            "node 2",
            ui.font(),
            ui::COLOR_BLACK,
            ui::COLOR_ORANGE_RED);
        m_tree_node_2->link(m_tree_node->container());

        m_tree_panel_2 = std::make_unique<ui::panel>(ui::COLOR_DARK_ORCHID);
        m_tree_panel_2->resize(50.0f, 50.0f);
        m_tree_panel_2->link(m_tree_node_2->container());
    }

    void initialize_camera()
    {
        auto& world = system<ecs::world>();
        auto& scene = system<scene::scene>();
        auto& relation = system<core::relation>();
        auto& graphics = system<graphics::graphics>();

        m_camera = world.create();
        world.add<core::link, graphics::camera, graphics::main_camera, scene::transform>(m_camera);
        auto& c_camera = world.component<graphics::camera>(m_camera);
        c_camera.parameter = graphics.make_pipeline_parameter("ash_pass");

        auto& c_transform = world.component<scene::transform>(m_camera);
        c_transform.position = {0.0f, 0.0f, -38.0f};
        c_transform.world_matrix = math::matrix_plain::affine_transform(
            c_transform.scaling,
            c_transform.rotation,
            c_transform.position);
        c_transform.dirty = true;

        relation.link(m_camera, scene.root());

        auto window_extent = system<window::window>().extent();
        resize_camera(window_extent.width, window_extent.height);
    }

    void resize_camera(std::uint32_t width, std::uint32_t height)
    {
        auto& world = system<ecs::world>();
        auto& graphics = system<graphics::graphics>();

        auto& camera = world.component<graphics::camera>(m_camera);
        camera.set(
            math::to_radians(45.0f),
            static_cast<float>(width) / static_cast<float>(height),
            0.3f,
            1000.0f,
            false);

        graphics::render_target_info render_target_info = {};
        render_target_info.width = width;
        render_target_info.height = height;
        render_target_info.format = graphics.back_buffer_format();
        render_target_info.samples = 4;
        m_render_target = graphics.make_render_target(render_target_info);
        camera.render_target = m_render_target.get();

        graphics::depth_stencil_buffer_info depth_stencil_buffer_info = {};
        depth_stencil_buffer_info.width = width;
        depth_stencil_buffer_info.height = height;
        depth_stencil_buffer_info.format = graphics::resource_format::D24_UNORM_S8_UINT;
        depth_stencil_buffer_info.samples = 4;
        m_depth_stencil_buffer = graphics.make_depth_stencil_buffer(depth_stencil_buffer_info);
        camera.depth_stencil_buffer = m_depth_stencil_buffer.get();
    }

    void update()
    {
        auto& world = system<ecs::world>();
        auto& scene = system<scene::scene>();
        auto& ui = system<ui::ui>();

        scene.reset_sync_counter();

        static float h = 0.0f;

        auto& keyboard = system<window::window>().keyboard();
        if (keyboard.key(window::KEYBOARD_KEY_Q).down())
        {
            m_panel->resize(200, h);
            h += 0.05f;
        }

        ui.tick();
    }

    std::unique_ptr<ui::label> m_text;
    std::unique_ptr<ui::panel> m_panel;

    std::unique_ptr<ui::tree> m_tree;
    std::unique_ptr<ui::tree_node> m_tree_node;
    std::unique_ptr<ui::panel> m_tree_panel;
    std::unique_ptr<ui::tree_node> m_tree_node_2;
    std::unique_ptr<ui::panel> m_tree_panel_2;

    ecs::entity m_camera;
    std::unique_ptr<graphics::resource> m_render_target;
    std::unique_ptr<graphics::resource> m_depth_stencil_buffer;
};

class ui_app
{
public:
    ui_app() : m_app("resource/config") {}

    void initialize()
    {
        m_app.install<window::window>();
        m_app.install<core::relation>();
        m_app.install<scene::scene>();
        m_app.install<graphics::graphics>();
        m_app.install<ui::ui>();
        m_app.install<test_system>();
    }

    void run() { m_app.run(); }

private:
    core::application m_app;
};
} // namespace ash::sample

int main()
{
    ash::sample::ui_app app;
    app.initialize();
    app.run();
    return 0;
}
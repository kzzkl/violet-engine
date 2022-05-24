#include "core/application.hpp"
#include "assert.hpp"
#include "graphics/graphics.hpp"
#include "core/relation.hpp"
#include "scene/scene.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"
#include "window/window_event.hpp"

namespace ash::sample::ui
{
class test_system : public core::system_base
{
public:
    test_system() : core::system_base("test") {}

    virtual bool initialize(const dictionary& config) override
    {
        initialize_task();
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
            graphics.begin_frame();
            graphics.render(m_camera);
            graphics.end_frame();
        });

        window_task->add_dependency(*task.find("root"));
        update_task->add_dependency(*window_task);
        render_task->add_dependency(*update_task);
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

        auto window_rect = system<window::window>().rect();
        resize_camera(window_rect.width, window_rect.height);
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
        auto& world = system<ash::ecs::world>();
        auto& scene = system<scene::scene>();
        scene.reset_sync_counter();

        auto& ui = system<ash::ui::ui>();
        ui.begin_frame();
        ui.texture(nullptr, {100, 100, 500, 500});
        ui.text("hello world! qap", {0, 0, 100, 40});
        ui.end_frame();
    }

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
        m_app.install<ash::ui::ui>();
        m_app.install<test_system>();
    }

    void run() { m_app.run(); }

private:
    core::application m_app;
};
} // namespace ash::sample::ui

int main()
{
    ash::sample ::ui::ui_app app;
    app.initialize();
    app.run();
    return 0;
}
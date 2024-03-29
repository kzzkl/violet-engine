#include "common/assert.hpp"
#include "core/application.hpp"
#include "core/relation.hpp"
#include "gallery.hpp"
#include "graphics/camera.hpp"
#include "graphics/graphics.hpp"
#include "graphics/graphics_event.hpp"
#include "graphics/rhi.hpp"
#include "scene/scene.hpp"
#include "task/task_manager.hpp"
#include "ui/ui.hpp"
#include "window/window.hpp"

namespace violet::sample
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

        system<core::event>().subscribe<graphics::event_render_extent_change>(
            "sample_module",
            [this](std::uint32_t width, std::uint32_t height) { resize_camera(width, height); });

        return true;
    }

private:
    void initialize_task()
    {
        auto& task = system<task::task_manager>();

        auto update_task = task.schedule("test update", [this]() { update(); });
        update_task->add_dependency(*task.find(task::TASK_GAME_LOGIC_START));
        task.find(task::TASK_GAME_LOGIC_END)->add_dependency(*update_task);
    }

    void initialize_ui()
    {
        m_gallery = std::make_unique<gallery>();
        m_gallery->initialize();
    }

    void initialize_camera()
    {
        auto& world = system<ecs::world>();
        auto& scene = system<scene::scene>();
        auto& relation = system<core::relation>();
        auto& graphics = system<graphics::graphics>();

        m_camera = world.create();
        world.add<core::link, graphics::camera, scene::transform>(m_camera);

        auto& c_transform = world.component<scene::transform>(m_camera);
        c_transform.position(math::float3{0.0f, 0.0f, -38.0f});

        relation.link(m_camera, scene.root());

        auto extent = graphics.render_extent();
        resize_camera(extent.width, extent.height);

        graphics.game_camera(m_camera);
    }

    void resize_camera(std::uint32_t width, std::uint32_t height)
    {
        auto& world = system<ecs::world>();
        auto& graphics = system<graphics::graphics>();

        auto& camera = world.component<graphics::camera>(m_camera);

        graphics::render_target_desc render_target = {};
        render_target.width = width;
        render_target.height = height;
        render_target.format = graphics::rhi::back_buffer_format();
        render_target.samples = 4;
        m_render_target = graphics::rhi::make_render_target(render_target);
        camera.render_target(m_render_target.get());

        graphics::depth_stencil_buffer_desc depth_stencil_buffer = {};
        depth_stencil_buffer.width = width;
        depth_stencil_buffer.height = height;
        depth_stencil_buffer.format = graphics::RESOURCE_FORMAT_D24_UNORM_S8_UINT;
        depth_stencil_buffer.samples = 4;
        m_depth_stencil_buffer = graphics::rhi::make_depth_stencil_buffer(depth_stencil_buffer);
        camera.depth_stencil_buffer(m_depth_stencil_buffer.get());
    }

    void update() {}

    ecs::entity m_camera;
    std::unique_ptr<graphics::resource_interface> m_render_target;
    std::unique_ptr<graphics::resource_interface> m_depth_stencil_buffer;

    std::unique_ptr<gallery> m_gallery;
};

class ui_app
{
public:
    ui_app() : m_app("ui/config") {}

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
} // namespace violet::sample

int main()
{
    violet::sample::ui_app app;
    app.initialize();
    app.run();
    return 0;
}
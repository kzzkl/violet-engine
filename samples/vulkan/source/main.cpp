#include "application.hpp"
#include "context.hpp"
#include "new_graphics_interface.hpp"
#include "plugin.hpp"
#include "window.hpp"

namespace ash::sample
{
class vulkan_plugin : public core::plugin
{
public:
    graphics::vk::factory& factory() { return *m_factory; }

protected:
    virtual bool do_load() override
    {
        graphics::vk::make_factory make =
            static_cast<graphics::vk::make_factory>(find_symbol("make_factory"));
        if (make == nullptr)
        {
            log::error("Symbol not found in plugin: make_context.");
            return false;
        }

        m_factory.reset(make());
        return true;
    }

private:
    std::unique_ptr<graphics::vk::factory> m_factory;
};

class test_system : public core::system_base
{
public:
    struct vertex
    {
        math::float2 position;
        math::float3 color;
    };

public:
    test_system() : core::system_base("test") {}

    virtual bool initialize(const dictionary& config) override
    {
        initialize_vulkan();
        initialize_pass();
        initialize_task();
        return true;
    }

private:
    void initialize_pass()
    {
        graphics::vk::render_pipeline_desc subpass_desc = {};
        subpass_desc.vertex_shader = "vert.spv";
        subpass_desc.pixel_shader = "frag.spv";
        std::size_t o = 0;
        subpass_desc.output = &o;
        subpass_desc.output_count = 1;

        graphics::vk::vertex_attribute_desc vertex_attributes[] = {
            {graphics::vk::vertex_attribute_type::FLOAT2, 0},
            {graphics::vk::vertex_attribute_type::FLOAT3, 8}
        };
        subpass_desc.vertex_layout.attributes = vertex_attributes;
        subpass_desc.vertex_layout.attribute_count = 2;

        std::vector<graphics::vk::render_pipeline_desc> subpasses;
        subpasses.push_back(subpass_desc);

        graphics::vk::render_pass_desc desc = {};
        desc.subpasses = subpasses.data();
        desc.subpass_count = subpasses.size();

        m_pass.reset(m_vulkan_plugin.factory().make_render_pass(desc));

        auto rect = system<window::window>().rect();
        for (std::size_t i = 0; i < m_renderer->back_buffer_count(); ++i)
        {
            std::array<graphics::vk::resource*, 1> frame_buffer_resources = {
                m_renderer->back_buffer(i)};
            graphics::vk::frame_buffer_desc frame_buffer_desc = {};
            frame_buffer_desc.render_pass = m_pass.get();
            frame_buffer_desc.resources = frame_buffer_resources.data();
            frame_buffer_desc.resource_count = frame_buffer_resources.size();
            frame_buffer_desc.width = rect.width;
            frame_buffer_desc.height = rect.height;
            m_frame_buffers.emplace_back(
                m_vulkan_plugin.factory().make_frame_buffer(frame_buffer_desc));
        }

        std::vector<vertex> vertices = {
            {{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
            {{0.5f, -0.5f},  {0.0f, 1.0f, 0.0f}},
            {{0.5f, 0.5f},   {0.0f, 0.0f, 1.0f}},
            {{-0.5f, 0.5f},  {1.0f, 1.0f, 1.0f}}
        };

        std::vector<std::uint32_t> indices = {0, 1, 2, 2, 3, 0};

        graphics::vk::vertex_buffer_desc vertex_buffer_desc = {};
        vertex_buffer_desc.vertex_size = sizeof(vertex);
        vertex_buffer_desc.vertex_count = vertices.size();
        vertex_buffer_desc.vertices = vertices.data();
        m_vertex_buffer.reset(m_vulkan_plugin.factory().make_vertex_buffer(vertex_buffer_desc));

        graphics::vk::index_buffer_desc index_buffer_desc = {};
        index_buffer_desc.index_size = sizeof(std::uint32_t);
        index_buffer_desc.index_count = indices.size();
        index_buffer_desc.indices = indices.data();
        m_index_buffer.reset(m_vulkan_plugin.factory().make_index_buffer(index_buffer_desc));
    }

    void initialize_task()
    {
        auto& task = system<task::task_manager>();
        auto& window = system<window::window>();

        auto window_task = task.schedule(
            "update",
            [&, this]() { window.tick(); },
            task::task_type::MAIN_THREAD);
        window_task->add_dependency(*task.find("root"));

        auto render_task = task.schedule("render", [&, this]() {
            std::size_t index = m_renderer->begin_frame();

            auto command = m_renderer->allocate_command();
            command->begin(m_pass.get(), m_frame_buffers[index].get());

            command->begin(m_pass->subpass(0));
            command->draw(m_vertex_buffer.get(), m_index_buffer.get(), 0, 6, 0);
            command->end(m_pass->subpass(0));

            command->end(m_pass.get());
            m_renderer->execute(command);

            m_renderer->end_frame();
        });
        render_task->add_dependency(*window_task);
    }

    void initialize_vulkan()
    {
        m_vulkan_plugin.load("ash-graphics-vulkan.dll");

        auto rect = system<window::window>().rect();

        graphics::vk::renderer_desc desc;
        desc.width = rect.width;
        desc.height = rect.height;
        desc.window_handle = system<window::window>().handle();

        m_renderer.reset(m_vulkan_plugin.factory().make_renderer(desc));
    }

    vulkan_plugin m_vulkan_plugin;

    std::unique_ptr<graphics::vk::renderer> m_renderer;
    std::unique_ptr<graphics::vk::render_pass> m_pass;

    std::unique_ptr<graphics::vk::resource> m_vertex_buffer;
    std::unique_ptr<graphics::vk::resource> m_index_buffer;

    std::vector<std::unique_ptr<graphics::vk::frame_buffer>> m_frame_buffers;
};

class vulkan_app
{
public:
    void initialize()
    {
        m_app.install<window::window>();
        m_app.install<test_system>();
    }

    void run() { m_app.run(); }

private:
    core::application m_app;
};
} // namespace ash::sample

int main()
{
    ash::sample::vulkan_app app;
    app.initialize();
    app.run();

    return 0;
}
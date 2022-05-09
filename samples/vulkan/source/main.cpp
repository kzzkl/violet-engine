#include "application.hpp"
#include "camera.hpp"
#include "context.hpp"
#include "graphics_interface.hpp"
#include "plugin.hpp"
#include "window.hpp"

namespace ash::sample
{
class vulkan_plugin : public core::plugin
{
public:
    graphics::factory_interface& factory() { return *m_factory; }

protected:
    virtual bool do_load() override
    {
        graphics::make_factory make =
            static_cast<graphics::make_factory>(find_symbol("make_factory"));
        if (make == nullptr)
        {
            log::error("Symbol not found in plugin: make_context.");
            return false;
        }

        m_factory.reset(make());
        return true;
    }

private:
    std::unique_ptr<graphics::factory_interface> m_factory;
};

class test_system : public core::system_base
{
public:
    struct vertex
    {
        math::float3 position;
        math::float3 color;
        math::float2 uv;
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
        graphics::pass_desc subpass_desc = {};
        subpass_desc.vertex_shader = "vert.spv";
        subpass_desc.pixel_shader = "frag.spv";
        std::size_t o = 0;
        subpass_desc.output = &o;
        subpass_desc.output_count = 1;

        std::vector<graphics::vertex_attribute_type> vertex_attributes = {
            graphics::vertex_attribute_type::FLOAT3,
            graphics::vertex_attribute_type::FLOAT3,
            graphics::vertex_attribute_type::FLOAT2};
        subpass_desc.vertex_layout.attributes = vertex_attributes.data();
        subpass_desc.vertex_layout.attribute_count = vertex_attributes.size();

        // Layout.
        std::vector<graphics::pass_parameter_pair> parameter_pairs_1 = {
            {graphics::pass_parameter_type::FLOAT3,   1},
            {graphics::pass_parameter_type::FLOAT4x4, 1},
            {graphics::pass_parameter_type::TEXTURE,  1}
        };
        graphics::pass_parameter_layout_desc parameter_layout_desc_1 = {};
        parameter_layout_desc_1.parameters = parameter_pairs_1.data();
        parameter_layout_desc_1.size = parameter_pairs_1.size();
        m_pass_parameter_layout_1.reset(
            m_vulkan_plugin.factory().make_pass_parameter_layout(parameter_layout_desc_1));

        m_parameter_1.reset(
            m_vulkan_plugin.factory().make_pass_parameter(m_pass_parameter_layout_1.get()));

        std::vector<graphics::pass_parameter_pair> parameter_pairs_2 = {
            {graphics::pass_parameter_type::FLOAT3, 1}
        };
        graphics::pass_parameter_layout_desc parameter_layout_desc_2 = {};
        parameter_layout_desc_2.parameters = parameter_pairs_2.data();
        parameter_layout_desc_2.size = parameter_pairs_2.size();
        m_pass_parameter_layout_2.reset(
            m_vulkan_plugin.factory().make_pass_parameter_layout(parameter_layout_desc_2));

        m_parameter_2.reset(
            m_vulkan_plugin.factory().make_pass_parameter(m_pass_parameter_layout_2.get()));

        std::vector<graphics::pass_parameter_layout_interface*> pl = {
            m_pass_parameter_layout_1.get(),
            m_pass_parameter_layout_2.get()};

        graphics::pass_layout_desc layout_desc;
        layout_desc.parameters = pl.data();
        layout_desc.size = pl.size();
        m_pass_layout.reset(m_vulkan_plugin.factory().make_pass_layout(layout_desc));
        subpass_desc.pass_layout = m_pass_layout.get();

        std::vector<graphics::pass_desc> subpasses;
        subpasses.push_back(subpass_desc);

        graphics::technique_desc desc = {};
        desc.subpasses = subpasses.data();
        desc.subpass_count = subpasses.size();

        m_pass.reset(m_vulkan_plugin.factory().make_technique(desc));

        auto rect = system<window::window>().rect();
        for (std::size_t i = 0; i < m_renderer->back_buffer_count(); ++i)
        {
            std::array<graphics::resource_interface*, 1> attachments = {m_renderer->back_buffer(i)};
            graphics::attachment_set_desc attachment_set_desc = {};
            attachment_set_desc.technique = m_pass.get();
            attachment_set_desc.attachments = attachments.data();
            attachment_set_desc.attachment_count = attachments.size();
            attachment_set_desc.width = rect.width;
            attachment_set_desc.height = rect.height;
            m_attachment_sets.emplace_back(
                m_vulkan_plugin.factory().make_attachment_set(attachment_set_desc));
        }

        std::vector<vertex> vertices = {
            {{-0.5f, 0.5f, 10.0f},  {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
            {{0.5f, 0.5f, 10.0f},   {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
            {{0.5f, -0.5f, 10.0f},  {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
            {{-0.5f, -0.5f, 10.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
        };

        std::vector<std::uint32_t> indices = {0, 1, 2, 2, 3, 0};

        graphics::vertex_buffer_desc vertex_buffer_desc = {};
        vertex_buffer_desc.vertex_size = sizeof(vertex);
        vertex_buffer_desc.vertex_count = vertices.size();
        vertex_buffer_desc.vertices = vertices.data();
        m_vertex_buffer.reset(m_vulkan_plugin.factory().make_vertex_buffer(vertex_buffer_desc));

        graphics::index_buffer_desc index_buffer_desc = {};
        index_buffer_desc.index_size = sizeof(std::uint32_t);
        index_buffer_desc.index_count = indices.size();
        index_buffer_desc.indices = indices.data();
        m_index_buffer.reset(m_vulkan_plugin.factory().make_index_buffer(index_buffer_desc));

        // m_texture.reset(m_vulkan_plugin.factory().make_texture("test_image.jpg"));
        m_texture.reset(m_vulkan_plugin.factory().make_texture("test_image.dds"));

        graphics::camera camera;
        camera.set(math::to_radians(30.0f), 1300.0f / 800.0f, 0.01f, 1000.0f, true);
        m_parameter_1->set(1, camera.projection);
        m_parameter_1->set(2, m_texture.get());

        m_parameter_2->set(0, math::float3{1.0f, 0.5f, 0.5f});
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
            m_color[0] += 0.0005f;
            m_color[1] += 0.0005f;
            m_color[2] += 0.0005f;

            std::size_t index = m_renderer->begin_frame();

            auto command = m_renderer->allocate_command();
            command->begin(m_pass.get(), m_attachment_sets[index].get());

            m_parameter_1->set(0, m_color);
            command->parameter(0, m_parameter_1.get());
            command->parameter(1, m_parameter_2.get());
            command->draw(m_vertex_buffer.get(), m_index_buffer.get(), 0, 6, 0);

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

        graphics::renderer_desc desc;
        desc.width = rect.width;
        desc.height = rect.height;
        desc.window_handle = system<window::window>().handle();

        m_renderer.reset(m_vulkan_plugin.factory().make_renderer(desc));
    }

    vulkan_plugin m_vulkan_plugin;

    std::unique_ptr<graphics::renderer_interface> m_renderer;
    std::unique_ptr<graphics::technique_interface> m_pass;

    std::unique_ptr<graphics::pass_parameter_layout_interface> m_pass_parameter_layout_1;
    std::unique_ptr<graphics::pass_parameter_layout_interface> m_pass_parameter_layout_2;
    std::unique_ptr<graphics::pass_layout_interface> m_pass_layout;

    std::unique_ptr<graphics::resource_interface> m_vertex_buffer;
    std::unique_ptr<graphics::resource_interface> m_index_buffer;

    std::vector<std::unique_ptr<graphics::attachment_set_interface>> m_attachment_sets;

    math::float3 m_color{};
    std::unique_ptr<graphics::pass_parameter_interface> m_parameter_1;
    std::unique_ptr<graphics::pass_parameter_interface> m_parameter_2;

    std::unique_ptr<graphics::resource_interface> m_texture;
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
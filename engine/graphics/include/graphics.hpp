#pragma once

#include "camera.hpp"
#include "config_parser.hpp"
#include "context.hpp"
#include "graphics_exports.hpp"
#include "graphics_plugin.hpp"
#include "render_parameter.hpp"
#include "render_pipeline.hpp"
#include "transform.hpp"
#include "type_trait.hpp"
#include "view.hpp"
#include "visual.hpp"

namespace ash::graphics
{
class GRAPHICS_API graphics : public ash::core::submodule
{
public:
    static constexpr uuid id = "cb3c4adc-4849-4871-8857-9ee68a9049e2";

    template <typename T>
    using interface_map = std::unordered_map<std::string, std::unique_ptr<T>>;

public:
    graphics() noexcept;

    virtual bool initialize(const dictionary& config) override;

    render_pipeline* make_render_pipeline(std::string_view name);

    std::unique_ptr<render_parameter> make_render_parameter(std::string_view name)
    {
        auto [fount, desc, _] = m_config.find_desc<pipeline_parameter_desc>(name);
        if (!fount)
        {
            log::error("render parameter no found: {}", name);
            return nullptr;
        }

        return std::make_unique<render_parameter>(m_factory->make_pipeline_parameter(desc));
    }

    template <typename Vertex>
    std::unique_ptr<resource> make_vertex_buffer(const Vertex* data, std::size_t size)
    {
        vertex_buffer_desc desc = {data, sizeof(Vertex), size};
        return std::unique_ptr<resource>(m_factory->make_vertex_buffer(desc));
    }

    template <typename Index>
    std::unique_ptr<resource> make_index_buffer(const Index* data, std::size_t size)
    {
        index_buffer_desc desc = {data, sizeof(Index), size};
        return std::unique_ptr<resource>(m_factory->make_index_buffer(desc));
    }

    std::unique_ptr<resource> make_texture(std::string_view file);

private:
    bool initialize_resource();
    void render();

    void update_pass_data();

    graphics_plugin m_plugin;
    renderer* m_renderer;
    factory* m_factory;

    ash::ecs::view<visual, scene::transform>* m_view;
    ash::ecs::view<main_camera, camera, scene::transform>* m_camera_view;

    std::unique_ptr<render_parameter> m_parameter_pass;

    interface_map<render_pipeline> m_render_pipeline;
    interface_map<render_parameter> m_parameters;

    config_parser m_config;
};
} // namespace ash::graphics
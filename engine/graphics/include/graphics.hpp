#pragma once

#include "camera.hpp"
#include "config_parser.hpp"
#include "context.hpp"
#include "graphics_exports.hpp"
#include "graphics_plugin.hpp"
#include "material.hpp"
#include "render_group.hpp"
#include "render_parameter.hpp"
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

    render_group* group(std::string_view name);

    template <typename... Types>
    std::unique_ptr<render_parameter<Types...>> make_render_parameter(std::string_view name)
    {
        auto [found, desc, _] = m_config.find_desc<pipeline_parameter_desc>(name);
        if (!found)
            return nullptr;

        std::vector<render_parameter_resource> resources;
        for (std::size_t i = 0; i < m_config.frame_resource(); ++i)
        {
            render_parameter_resource resource;
            resource.parameter = m_factory->make_pipeline_parameter(desc);

            type_list<Types...>::each([&resource, this ]<typename T>() {
                resource.data.push_back(m_factory->make_upload_buffer(sizeof(T)));
            });

            resources.push_back(resource);
        }

        return std::make_unique<render_parameter<Types...>>(resources);
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

private:
    bool initialize_resource();
    void render();

    void update_pass_data();

    render_group* make_render_group(std::string_view name);

    graphics_plugin m_plugin;
    renderer* m_renderer;
    factory* m_factory;

    ash::ecs::view<visual, mesh, material>* m_view;
    ash::ecs::view<main_camera, camera, scene::transform>* m_camera_view;

    std::unique_ptr<render_parameter_pass> m_parameter_pass;

    interface_map<render_group> m_render_group;

    config_parser m_config;
};
} // namespace ash::graphics
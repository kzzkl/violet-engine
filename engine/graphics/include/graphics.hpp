#pragma once

#include "camera.hpp"
#include "config_parser.hpp"
#include "context.hpp"
#include "graphics_exports.hpp"
#include "graphics_plugin.hpp"
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

    render_group* make_render_group(std::string_view name);

    render_parameter_base* make_render_parameter(std::string_view name)
    {
        auto iter = m_parameters.find(name.data());
        if (iter != m_parameters.end())
            return iter->second.get();
        else
            return nullptr;
    }

    template <typename... Types>
    render_parameter<Types...>* make_render_parameter(std::string_view name)
    {
        render_parameter_base* f = make_render_parameter(name);
        if (f != nullptr)
            return dynamic_cast<render_parameter<Types...>*>(f);

        pipeline_parameter_desc desc = {};
        std::vector<resource*> part;
        type_list<Types...>::each([&desc, &part, this]<typename T>() {
            using resource_type = T::value_type;

            if constexpr (std::is_same_v<resource_type, texture>)
            {
                // TODO: make texture
                desc.data[desc.size] = pipeline_parameter_type::TEXTURE;
            }
            else
            {
                std::size_t buffer_size = sizeof(resource_type);
                if constexpr (T::frame_resource)
                    buffer_size *= m_config.frame_resource();

                part.push_back(m_factory->make_upload_buffer(buffer_size));

                desc.data[desc.size] = pipeline_parameter_type::BUFFER;
            }

            ++desc.size;
        });

        std::vector<pipeline_parameter*> parameter;
        for (std::size_t i = 0; i < m_config.frame_resource(); ++i)
            parameter.push_back(m_factory->make_pipeline_parameter(desc));

        auto temp = std::make_unique<render_parameter<Types...>>(parameter, part);
        auto result = temp.get();
        m_parameters[name.data()] = std::move(temp);
        return result;
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

    graphics_plugin m_plugin;
    renderer* m_renderer;
    factory* m_factory;

    ash::ecs::view<visual, mesh, scene::transform>* m_view;
    ash::ecs::view<main_camera, camera, scene::transform>* m_camera_view;

    render_parameter_pass* m_parameter_pass;

    interface_map<render_group> m_render_group;
    interface_map<render_parameter_base> m_parameters;

    config_parser m_config;
};
} // namespace ash::graphics
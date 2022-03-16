#pragma once

#include "config_parser.hpp"
#include "context.hpp"
#include "graphics_exports.hpp"
#include "graphics_plugin.hpp"
#include "material.hpp"
#include "render_group.hpp"
#include "view.hpp"
#include "visual.hpp"

namespace ash::graphics
{
class GRAPHICS_API graphics : public ash::core::submodule
{
public:
    static constexpr uuid id = "cb3c4adc-4849-4871-8857-9ee68a9049e2";

public:
    graphics() noexcept;

    virtual bool initialize(const dictionary& config) override;

    render_group* get_group(std::string_view name);

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

    graphics_plugin m_plugin;
    renderer* m_renderer;
    factory* m_factory;

    ash::ecs::view<visual, mesh, material>* m_view;
    std::unordered_map<std::string, std::unique_ptr<render_group>> m_render_group;

    pipeline_parameter_layout* m_layout;

    pipeline_parameter* m_parameter_object;
    pipeline_parameter* m_parameter_material;
    resource* m_mvp;
    resource* m_material;

    config_parser m_config;
};
} // namespace ash::graphics
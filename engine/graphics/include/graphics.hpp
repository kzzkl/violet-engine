#pragma once

#include "config_parser.hpp"
#include "context.hpp"
#include "graphics_exports.hpp"
#include "graphics_plugin.hpp"

namespace ash::graphics
{
class GRAPHICS_API graphics : public ash::core::submodule
{
public:
    static constexpr uuid id = "cb3c4adc-4849-4871-8857-9ee68a9049e2";

public:
    graphics() noexcept;

    virtual bool initialize(const dictionary& config) override;

private:
    bool initialize_resource();
    void render();

    graphics_plugin m_plugin;
    renderer* m_renderer;
    factory* m_factory;

    pipeline_parameter_layout* m_layout;
    pipeline* m_pipeline;
    resource* m_vertices;
    resource* m_indices;

    pipeline_parameter* m_parameter_object;
    pipeline_parameter* m_parameter_material;
    resource* m_mvp;
    resource* m_material;

    config_parser m_config;
};
} // namespace ash::graphics
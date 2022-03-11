#pragma once

#include "graphics_exports.hpp"
#include "graphics_plugin.hpp"
#include "submodule.hpp"

namespace ash::graphics
{
class GRAPHICS_API graphics : public ash::core::submodule
{
public:
    static constexpr ash::core::uuid id = "cb3c4adc-4849-4871-8857-9ee68a9049e2";

public:
    graphics();

    virtual bool initialize(const ash::common::dictionary& config) override;

private:
    void initialize_resource();

    void render();

    context_config get_config(const ash::common::dictionary& config);

    graphics_plugin m_plugin;
    renderer* m_renderer;
    factory* m_factory;

    pipeline_parameter_layout* m_layout;
    pipeline* m_pipeline;
    resource* m_vertices;
    resource* m_indices;

    pipeline_parameter* m_parameter;
    resource* m_mvp;
};
} // namespace ash::graphics
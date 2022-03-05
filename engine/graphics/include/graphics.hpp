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
    using renderer = ash::graphics::external::renderer;
    using config = ash::graphics::external::graphics_context_config;

public:
    graphics();

    virtual bool initialize(const ash::common::dictionary& config) override;

private:
    void initialize_task();
    config get_config(const ash::common::dictionary& config);

    graphics_plugin m_plugin;
    renderer* m_renderer;
};
} // namespace ash::graphics
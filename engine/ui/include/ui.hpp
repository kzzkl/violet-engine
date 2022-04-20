#pragma once

#include "context.hpp"
#include "graphics.hpp"
#include "ui_impl.hpp"

namespace ash::ui
{
class ui : public core::system_base
{
public:
    ui();

    virtual bool initialize(const dictionary& config) override;

    void begin_frame();
    void end_frame();

    void test() { m_impl->test(); }

private:
    void render();

    ecs::entity m_ui_entity;

    std::unique_ptr<ui_impl> m_impl;
    std::unique_ptr<graphics::render_pipeline> m_pipeline;
};
}; // namespace ash::ui
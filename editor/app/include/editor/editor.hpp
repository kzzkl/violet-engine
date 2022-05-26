#pragma once

#include "core/application.hpp"
#include "editor/editor_ui.hpp"
#include "graphics_interface.hpp"

namespace ash::editor
{
class editor : public core::system_base
{
public:
    editor();

    virtual bool initialize(const dictionary& config) override;

private:
    void initialize_task();
    void initialize_camera();
    void draw();

    void resize(std::uint32_t width, std::uint32_t height);

    void test_update();

    ecs::entity m_editor_camera;
    std::unique_ptr<graphics::resource> m_render_target;
    std::unique_ptr<graphics::resource> m_depth_stencil_buffer;

    std::unique_ptr<editor_ui> m_ui;
};

class editor_app
{
public:
    editor_app();

    void initialize();
    void run();

private:
    core::application m_app;
};
} // namespace ash::editor
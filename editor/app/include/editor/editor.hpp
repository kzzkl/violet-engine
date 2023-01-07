#pragma once

#include "core/application.hpp"
#include "editor/editor_ui.hpp"
#include "graphics_interface.hpp"

namespace violet::editor
{
class editor : public core::system_base
{
public:
    editor();

    virtual bool initialize(const dictionary& config) override;

private:
    void initialize_task();
    void initialize_camera();

    void resize(std::uint32_t width, std::uint32_t height);

    ecs::entity m_editor_camera;
    std::unique_ptr<graphics::resource_interface> m_render_target;
    std::unique_ptr<graphics::resource_interface> m_depth_stencil_buffer;

    std::unique_ptr<editor_ui> m_ui;
};
} // namespace violet::editor
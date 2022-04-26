#pragma once

#include "application.hpp"

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

    void test_update();

    ecs::entity m_editor_camera;
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
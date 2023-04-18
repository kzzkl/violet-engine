#pragma once

#include "interface/graphics_interface.hpp"
#include <vector>

namespace violet
{
class mesh;
class render_pipeline
{
public:
    void add_mesh(mesh* mesh);
    void reset();

    void render(render_command_interface* command);

protected:
    virtual void on_render(const std::vector<mesh*>& meshes, render_command_interface* command) = 0;

private:
    std::vector<mesh*> m_meshes;
};
} // namespace violet
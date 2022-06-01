#pragma once

#include "camera.hpp"
#include "graphics/visual.hpp"
#include "graphics_interface.hpp"
#include "pipeline_parameter.hpp"
#include <vector>

namespace ash::graphics
{
struct render_unit
{
    std::vector<resource_interface*> vertex_buffers;
    resource_interface* index_buffer;

    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;
    std::vector<pipeline_parameter*> parameters;

    scissor_extent scissor;
};

class render_pipeline
{
public:
    render_pipeline();
    virtual ~render_pipeline() = default;

    void add(const visual& visual, std::size_t submesh_index);
    void clear() { m_units.clear(); }

    virtual void render(const camera& camera, render_command_interface* command) = 0;

protected:
    const std::vector<render_unit>& units() const { return m_units; }

private:
    std::vector<render_unit> m_units;
};
} // namespace ash::graphics
#pragma once

#include "graphics_interface.hpp"
#include "mesh.hpp"
#include "render_parameter.hpp"
#include "visual.hpp"
#include <vector>

namespace ash::graphics
{
struct render_unit
{
    mesh* mesh;
    visual* visual;
};

class render_group
{
public:
    using layout_type = pipeline_layout;
    using pipeline_type = pipeline;

public:
    render_group(
        layout_type* layout,
        pipeline_type* pipeline,
        std::size_t unit_parameter_count,
        std::size_t group_parameter_count);

    void add(const render_unit& unit) { m_units.push_back(unit); }
    void clear() { m_units.clear(); }

    pipeline_type* pipeline() const noexcept { return m_pipeline.get(); }
    layout_type* layout() const noexcept { return m_layout.get(); }

    void render(render_command* command, resource* target);
    void parameter(std::size_t index, render_parameter_base* parameter);

private:
    std::unique_ptr<layout_type> m_layout;
    std::unique_ptr<pipeline_type> m_pipeline;

    std::vector<render_unit> m_units;

    std::vector<render_parameter_base*> m_parameters;

    std::size_t m_parameter_offset;
};
} // namespace ash::graphics
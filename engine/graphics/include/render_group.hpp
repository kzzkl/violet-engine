#pragma once

#include "graphics_interface.hpp"
#include "material.hpp"
#include "mesh.hpp"
#include "render_parameter.hpp"
#include <vector>

namespace ash::graphics
{
struct render_unit
{
    mesh* mesh;
    render_parameter_object* parameter;
};

class render_group
{
public:
    using layout_type = pipeline_layout;
    using pipeline_type = pipeline;

public:
    render_group(layout_type* layout, pipeline_type* pipeline);

    void add(const render_unit& unit) { m_units.push_back(unit); }
    void clear() { m_units.clear(); }

    pipeline_type* pipeline() const noexcept { return m_pipeline.get(); }
    layout_type* layout() const noexcept { return m_layout.get(); }

    auto begin() noexcept { return m_units.begin(); }
    auto end() noexcept { return m_units.end(); }

private:
    std::unique_ptr<layout_type> m_layout;
    std::unique_ptr<pipeline_type> m_pipeline;

    std::vector<render_unit> m_units;
};
} // namespace ash::graphics
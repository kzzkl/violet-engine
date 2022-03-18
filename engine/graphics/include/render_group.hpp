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
    render_group(pipeline_layout* layout, pipeline* pipeline);

    void add(const render_unit& unit) { m_units.push_back(unit); }
    void clear() { m_units.clear(); }

    pipeline* get_pipeline() const noexcept { return m_pipeline.get(); }
    pipeline_layout* get_layout() const noexcept { return m_layout.get(); }

    auto begin() noexcept { return m_units.begin(); }
    auto end() noexcept { return m_units.end(); }

private:
    std::unique_ptr<pipeline_layout> m_layout;
    std::unique_ptr<pipeline> m_pipeline;

    std::vector<render_unit> m_units;
};
} // namespace ash::graphics
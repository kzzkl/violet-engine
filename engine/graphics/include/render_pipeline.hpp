#pragma once

#include "graphics_interface.hpp"
#include "render_parameter.hpp"
#include "visual.hpp"
#include <vector>

namespace ash::graphics
{
struct render_unit
{
    resource* vertex_buffer;
    resource* index_buffer;

    std::size_t index_start;
    std::size_t index_end;

    pipeline_parameter* object;
    pipeline_parameter* material;
};

class render_pipeline
{
public:
    using layout_type = pipeline_layout;
    using pipeline_type = pipeline;

public:
    render_pipeline(layout_type* layout, pipeline_type* pipeline);

    void add(const render_unit& unit) { m_units.push_back(unit); }
    void clear() { m_units.clear(); }

    pipeline_type* pipeline() const noexcept { return m_pipeline.get(); }
    layout_type* layout() const noexcept { return m_layout.get(); }

    const std::vector<render_unit>& units() const { return m_units; }

private:
    std::unique_ptr<layout_type> m_layout;
    std::unique_ptr<pipeline_type> m_pipeline;

    std::vector<render_unit> m_units;

    std::size_t m_parameter_offset;
};
} // namespace ash::graphics
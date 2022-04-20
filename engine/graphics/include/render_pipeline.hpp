#pragma once

#include "graphics_interface.hpp"
#include "render_parameter.hpp"
#include <vector>

namespace ash::graphics
{
class render_pipeline;
struct render_unit
{
    resource* vertex_buffer;
    resource* index_buffer;

    std::size_t index_start;
    std::size_t index_end;
    std::size_t vertex_base;

    render_pipeline* pipeline;
    std::vector<render_parameter*> parameters;
};

class render_pipeline
{
public:
    using layout_type = pipeline_layout;
    using pipeline_type = pipeline;

public:
    render_pipeline(layout_type* layout, pipeline_type* pipeline);
    virtual ~render_pipeline() = default;

    void add(const render_unit* unit) { m_units.push_back(unit); }
    void clear() { m_units.clear(); }

    virtual void render(resource* target, render_command* command, render_parameter* pass);

    void parameter_count(std::size_t unit, std::size_t pass) noexcept
    {
        m_unit_parameter_count = unit;
        m_pass_parameter_count = pass;
    }

    std::size_t unit_parameter_count() const { return m_unit_parameter_count; }
    std::size_t pass_parameter_count() const { return m_pass_parameter_count; }

protected:
    pipeline_type* pipeline() const noexcept { return m_pipeline.get(); }
    layout_type* layout() const noexcept { return m_layout.get(); }

    const std::vector<const render_unit*>& units() const { return m_units; }

private:
    std::unique_ptr<layout_type> m_layout;
    std::unique_ptr<pipeline_type> m_pipeline;

    std::size_t m_unit_parameter_count;
    std::size_t m_pass_parameter_count;

    std::vector<const render_unit*> m_units;
};
} // namespace ash::graphics
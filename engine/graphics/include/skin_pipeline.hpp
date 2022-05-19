#pragma once

#include "pipeline_parameter.hpp"
#include "skinned_mesh.hpp"
#include "visual.hpp"
#include <vector>

namespace ash::graphics
{
struct skin_unit
{
    std::vector<resource*> input_buffers;
    std::vector<resource*> output_buffers;
    pipeline_parameter* parameter;

    std::size_t vertex_count;
};

class skin_pipeline
{
public:
    virtual ~skin_pipeline() = default;

    void add(const skinned_mesh& skinned_mesh);
    void clear() noexcept { m_units.clear(); }

    virtual void skin(render_command_interface* command) = 0;

protected:
    const std::vector<skin_unit>& units() const { return m_units; }

private:
    std::vector<skin_unit> m_units;
};
} // namespace ash::graphics
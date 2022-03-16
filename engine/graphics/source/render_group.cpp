#include "render_group.hpp"

namespace ash::graphics
{
render_group::render_group(pipeline_parameter_layout* layout, pipeline* pipeline)
    : m_layout(layout),
      m_pipeline(pipeline)
{
}
} // namespace ash::graphics
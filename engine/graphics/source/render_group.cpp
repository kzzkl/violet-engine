#include "render_group.hpp"

namespace ash::graphics
{
render_group::render_group(layout_type* layout, pipeline_type* pipeline)
    : m_layout(layout),
      m_pipeline(pipeline)
{
}
} // namespace ash::graphics
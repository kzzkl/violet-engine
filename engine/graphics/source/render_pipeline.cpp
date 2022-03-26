#include "render_pipeline.hpp"

namespace ash::graphics
{
render_pipeline::render_pipeline(layout_type* layout, pipeline_type* pipeline)
    : m_layout(layout),
      m_pipeline(pipeline)
{
}
} // namespace ash::graphics
#include "render_pipeline.hpp"

namespace ash::graphics
{
render_pass::render_pass(render_pass_interface* interface) : m_interface(interface)
{
}
} // namespace ash::graphics
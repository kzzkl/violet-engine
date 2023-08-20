#include "graphics/render_graph/render_resource.hpp"

namespace violet
{
render_resource::render_resource(std::string_view name, rhi_context* rhi)
    : render_node(name, rhi),
      m_format(RHI_RESOURCE_FORMAT_UNDEFINED)
{
}
} // namespace violet
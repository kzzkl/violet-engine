#include "graphics/render_graph/render_resource.hpp"

namespace violet
{
render_resource::render_resource(bool framebuffer_cache)
    : m_format(RHI_RESOURCE_FORMAT_UNDEFINED),
      m_samples(RHI_SAMPLE_COUNT_1),
      m_framebuffer_cache(framebuffer_cache)
{
}
} // namespace violet
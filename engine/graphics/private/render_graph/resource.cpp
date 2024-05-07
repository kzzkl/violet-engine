#include "graphics/render_graph/resource.hpp"

namespace violet
{
resource::resource(std::string_view name, bool external) : m_name(name), m_external(external)
{
}

resource::~resource()
{
}

texture::texture(std::string_view name, bool external)
    : resource(name, external),
      m_format(RHI_FORMAT_UNDEFINED),
      m_samples(RHI_SAMPLE_COUNT_1),
      m_texture(nullptr)
{
}

swapchain::swapchain(std::string_view name)
    : resource(name, true),
      m_format(RHI_FORMAT_UNDEFINED),
      m_samples(RHI_SAMPLE_COUNT_1),
      m_swapchain(nullptr)
{
}
} // namespace violet
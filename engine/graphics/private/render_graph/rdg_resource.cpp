#include "graphics/render_graph/rdg_resource.hpp"

namespace violet
{
rdg_resource::rdg_resource() : m_index(-1), m_external(false)
{
}

rdg_resource::~rdg_resource()
{
}

rdg_texture::rdg_texture() : m_format(RHI_FORMAT_UNDEFINED), m_samples(RHI_SAMPLE_COUNT_1)
{
}
} // namespace violet
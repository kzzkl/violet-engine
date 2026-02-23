#include "vk_query.hpp"
#include "vk_context.hpp"
#include "vk_utils.hpp"

namespace violet::vk
{
vk_query_pool::vk_query_pool(const rhi_query_pool_desc& desc, vk_context* context)
    : m_size(desc.size),
      m_context(context)
{
    m_query_type = vk_utils::map_query_type(desc.type);

    VkQueryPoolCreateInfo info = {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .queryType = m_query_type,
        .queryCount = desc.size,
    };

    vk_check(vkCreateQueryPool(m_context->get_device(), &info, nullptr, &m_query_pool));
}

vk_query_pool::~vk_query_pool()
{
    vkDestroyQueryPool(m_context->get_device(), m_query_pool, nullptr);
}

void vk_query_pool::reset()
{
    vkResetQueryPool(m_context->get_device(), m_query_pool, 0, m_size);
}

void vk_query_pool::get_results(std::uint64_t* data, std::uint32_t count)
{
    vkGetQueryPoolResults(
        m_context->get_device(),
        m_query_pool,
        0,
        count,
        count * sizeof(std::uint64_t),
        data,
        sizeof(std::uint64_t),
        VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
}
} // namespace violet::vk
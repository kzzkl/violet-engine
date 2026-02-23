#include "vk_common.hpp"

namespace violet::vk
{
class vk_context;
class vk_query_pool : public rhi_query_pool
{
public:
    vk_query_pool(const rhi_query_pool_desc& desc, vk_context* context);
    virtual ~vk_query_pool();

    void reset() override;

    void get_results(std::uint64_t* data, std::uint32_t count) override;

    std::uint32_t get_size() const noexcept override
    {
        return m_size;
    }

    VkQueryPool get_query_pool() const noexcept
    {
        return m_query_pool;
    }

    VkQueryType get_query_type() const noexcept
    {
        return m_query_type;
    }

private:
    VkQueryPool m_query_pool;
    VkQueryType m_query_type;
    std::uint32_t m_size;

    vk_context* m_context;
};
} // namespace violet::vk
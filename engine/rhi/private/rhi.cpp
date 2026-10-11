#include "rhi/rhi.hpp"

#ifdef VIOLET_ENABLE_VULKAN
#include "vulkan/vk_rhi.hpp"
#endif

namespace violet
{
rhi* rhi::create(rhi_backend backend)
{
    switch (backend)
    {
#ifdef VIOLET_ENABLE_VULKAN
    case RHI_BACKEND_VULKAN:
        return new vk::vk_rhi();
#endif
    default:
        return nullptr;
    }
}

void rhi::destroy(rhi* rhi)
{
    delete rhi;
}
} // namespace violet
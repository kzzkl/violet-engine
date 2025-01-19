#pragma once

#include "common/allocator.hpp"
#include "vk_parameter.hpp"
#include "vk_resource.hpp"
#include <mutex>

namespace violet::vk
{
class vk_bindless_manager
{
public:
    vk_bindless_manager(vk_context* context);

    std::uint32_t allocate_resource(vk_texture_srv* srv);
    std::uint32_t allocate_resource(vk_buffer_srv* srv);
    std::uint32_t allocate_resource(vk_texture_uav* uav);
    std::uint32_t allocate_resource(vk_buffer_uav* uav);
    void free_resource(std::uint32_t index);

    std::uint32_t allocate_sampler(vk_sampler* sampler);
    void free_sampler(std::uint32_t index);

    vk_parameter* get_bindless_parameter() const noexcept
    {
        return m_bindless_parameter.get();
    }

private:
    index_allocator<std::uint32_t> m_resource_allocator;
    index_allocator<std::uint32_t> m_sampler_allocator;
    std::mutex m_resource_allocator_mutex;
    std::mutex m_sampler_allocator_mutex;

    std::unique_ptr<vk_parameter> m_bindless_parameter;

    vk_context* m_context;
};
} // namespace violet::vk
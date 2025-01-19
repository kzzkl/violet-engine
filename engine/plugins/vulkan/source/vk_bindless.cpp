#include "vk_bindless.hpp"

namespace violet::vk
{
vk_bindless_manager::vk_bindless_manager(vk_context* context)
    : m_context(context)
{
    std::vector<rhi_parameter_binding> bindless_parameter_bindings = {
        {
            .type = RHI_PARAMETER_BINDING_MUTABLE,
            .stages = RHI_SHADER_STAGE_ALL,
            .size = 0,
        },
        {
            .type = RHI_PARAMETER_BINDING_SAMPLER,
            .stages = RHI_SHADER_STAGE_FRAGMENT | RHI_SHADER_STAGE_COMPUTE,
            .size = 128,
        },
    };

    rhi_parameter_desc bindless_parameter_desc = {
        .bindings = bindless_parameter_bindings.data(),
        .binding_count = static_cast<std::uint32_t>(bindless_parameter_bindings.size()),
        .flags = RHI_PARAMETER_SIMPLE | RHI_PARAMETER_DISABLE_SYNC,
    };

    m_bindless_parameter = std::make_unique<vk_parameter>(bindless_parameter_desc, m_context);
}

std::uint32_t vk_bindless_manager::allocate_resource(vk_texture_srv* srv)
{
    std::lock_guard<std::mutex> lock(m_resource_allocator_mutex);
    std::uint32_t index = m_resource_allocator.allocate();
    m_bindless_parameter->set_srv(0, srv, index);

    return index;
}

std::uint32_t vk_bindless_manager::allocate_resource(vk_buffer_srv* srv)
{
    std::lock_guard<std::mutex> lock(m_resource_allocator_mutex);
    std::uint32_t index = m_resource_allocator.allocate();
    m_bindless_parameter->set_srv(0, srv, index);

    return index;
}

std::uint32_t vk_bindless_manager::allocate_resource(vk_texture_uav* uav)
{
    std::lock_guard<std::mutex> lock(m_resource_allocator_mutex);
    std::uint32_t index = m_resource_allocator.allocate();
    m_bindless_parameter->set_uav(0, uav, index);

    return index;
}

std::uint32_t vk_bindless_manager::allocate_resource(vk_buffer_uav* uav)
{
    std::lock_guard<std::mutex> lock(m_resource_allocator_mutex);
    std::uint32_t index = m_resource_allocator.allocate();
    m_bindless_parameter->set_uav(0, uav, index);

    return index;
}

void vk_bindless_manager::free_resource(std::uint32_t index)
{
    std::lock_guard<std::mutex> lock(m_resource_allocator_mutex);
    m_resource_allocator.free(index);
}

std::uint32_t vk_bindless_manager::allocate_sampler(vk_sampler* sampler)
{
    std::lock_guard<std::mutex> lock(m_sampler_allocator_mutex);
    std::uint32_t index = m_sampler_allocator.allocate();
    m_bindless_parameter->set_sampler(1, sampler, index);

    return index;
}

void vk_bindless_manager::free_sampler(std::uint32_t index)
{
    if (index == RHI_INVALID_BINDLESS_HANDLE)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(m_sampler_allocator_mutex);
    m_sampler_allocator.free(index);
}
} // namespace violet::vk
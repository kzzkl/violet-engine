#include "vk_parameter.hpp"
#include "vk_context.hpp"
#include "vk_layout.hpp"
#include "vk_resource.hpp"

namespace violet::vk
{
vk_parameter::vk_parameter(const rhi_parameter_desc& desc, vk_context* context)
    : m_update_counts(desc.binding_count),
      m_flags(desc.flags),
      m_context(context)
{
    auto* layout_manager = m_context->get_layout_manager();
    auto* parameter_manager = m_context->get_parameter_manager();

    std::size_t copy_count =
        desc.flags & RHI_PARAMETER_SIMPLE ? 1 : m_context->get_frame_resource_count();

    m_layout = layout_manager->get_parameter_layout(desc);

    std::vector<VkWriteDescriptorSet> descriptor_write;
    std::vector<VkDescriptorBufferInfo> uniform_infos;
    uniform_infos.reserve(desc.binding_count * copy_count);

    VkBuffer uniform_buffer = parameter_manager->get_uniform_buffer();

    for (std::size_t i = 0; i < copy_count; ++i)
    {
        copy copy = {};
        copy.descriptor_set = m_context->allocate_descriptor_set(m_layout->get_layout());

        const auto& bindings = m_layout->get_bindings();
        for (std::size_t j = 0; j < bindings.size(); ++j)
        {
            const auto& binding = bindings[j];

            if (binding.type == RHI_PARAMETER_BINDING_UNIFORM)
            {
                copy.uniforms.push_back(parameter_manager->allocate_uniform(binding.size));

                uniform_infos.push_back({
                    .buffer = uniform_buffer,
                    .offset = copy.uniforms[binding.uniform.index].offset,
                    .range = binding.size,
                });

                descriptor_write.push_back({
                    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet = copy.descriptor_set,
                    .dstBinding = static_cast<std::uint32_t>(j),
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo = &uniform_infos.back(),
                });
            }
        }

        m_copies.push_back(copy);
    }

    if (!descriptor_write.empty())
    {
        vkUpdateDescriptorSets(
            m_context->get_device(),
            static_cast<std::uint32_t>(descriptor_write.size()),
            descriptor_write.data(),
            0,
            nullptr);
    }
}

vk_parameter::~vk_parameter()
{
    if (m_dirty)
    {
        m_context->get_parameter_manager()->remove_dirty_parameter(this);
    }

    for (auto& copy : m_copies)
    {
        for (auto& uniform : copy.uniforms)
        {
            m_context->get_parameter_manager()->free_uniform(uniform);
        }

        m_context->free_descriptor_set(copy.descriptor_set);
    }
}

void vk_parameter::set_uniform(
    std::uint32_t index,
    const void* data,
    std::size_t size,
    std::size_t offset)
{
    const auto& binding = m_layout->get_bindings()[index];
    assert(binding.type == RHI_PARAMETER_BINDING_UNIFORM);

    const auto& uniforms = get_uniforms();

    void* buffer =
        m_context->get_parameter_manager()->get_uniform_pointer(uniforms[index].offset + offset);

    std::memcpy(buffer, data, size);

    mark_dirty(index);
}

void vk_parameter::set_srv(std::uint32_t index, rhi_texture_srv* srv, std::uint32_t offset)
{
    const auto& binding = m_layout->get_bindings()[index];
    assert(
        binding.type == RHI_PARAMETER_BINDING_TEXTURE ||
        binding.type == RHI_PARAMETER_BINDING_MUTABLE);

    static_cast<vk_texture_srv*>(srv)->write(get_descriptor_set(), index, offset);

    mark_dirty(index);
}

void vk_parameter::set_srv(std::uint32_t index, rhi_buffer_srv* srv, std::uint32_t offset)
{
    const auto& binding = m_layout->get_bindings()[index];
    assert(binding.type == RHI_PARAMETER_BINDING_MUTABLE);

    static_cast<vk_buffer_srv*>(srv)->write(get_descriptor_set(), index, offset);

    mark_dirty(index);
}

void vk_parameter::set_uav(std::uint32_t index, rhi_texture_uav* uav, std::uint32_t offset)
{
    const auto& binding = m_layout->get_bindings()[index];
    assert(
        binding.type == RHI_PARAMETER_BINDING_STORAGE_TEXTURE ||
        binding.type == RHI_PARAMETER_BINDING_MUTABLE);

    static_cast<vk_texture_uav*>(uav)->write(get_descriptor_set(), index, offset);

    mark_dirty(index);
}

void vk_parameter::set_uav(std::uint32_t index, rhi_buffer_uav* uav, std::uint32_t offset)
{
    const auto& binding = m_layout->get_bindings()[index];
    assert(
        binding.type == RHI_PARAMETER_BINDING_STORAGE_BUFFER ||
        binding.type == RHI_PARAMETER_BINDING_STORAGE_TEXEL ||
        binding.type == RHI_PARAMETER_BINDING_MUTABLE);

    static_cast<vk_buffer_uav*>(uav)->write(get_descriptor_set(), index, offset);

    mark_dirty(index);
}

void vk_parameter::set_sampler(std::uint32_t index, rhi_sampler* sampler, std::uint32_t offset)
{
    const auto& binding = m_layout->get_bindings()[index];
    assert(binding.type == RHI_PARAMETER_BINDING_SAMPLER);

    static_cast<vk_sampler*>(sampler)->write(get_descriptor_set(), index, offset);

    mark_dirty(index);
}

bool vk_parameter::sync()
{
    std::size_t frame_resource_count = m_context->get_frame_resource_count();
    std::size_t frame_resource_index = m_context->get_frame_resource_index();

    copy& prev_copy =
        m_copies[(frame_resource_index + frame_resource_count - 1) % frame_resource_count];
    copy& curr_copy = m_copies[frame_resource_index];

    std::vector<VkCopyDescriptorSet> descriptor_copy;

    std::uint32_t remaining_update_count = 0;

    const auto& bindings = m_layout->get_bindings();
    for (std::size_t i = 0; i < bindings.size(); ++i)
    {
        const auto& binding = bindings[i];

        if (m_update_counts[i] == 0)
        {
            continue;
        }

        --m_update_counts[i];
        remaining_update_count += m_update_counts[i];

        if (binding.type == RHI_PARAMETER_BINDING_UNIFORM)
        {
            void* source = m_context->get_parameter_manager()->get_uniform_pointer(
                prev_copy.uniforms[binding.uniform.index].offset);
            void* target = m_context->get_parameter_manager()->get_uniform_pointer(
                curr_copy.uniforms[binding.uniform.index].offset);

            std::memcpy(target, source, binding.size);
        }
        else
        {
            VkCopyDescriptorSet copy = {
                .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
                .srcSet = prev_copy.descriptor_set,
                .srcBinding = static_cast<std::uint32_t>(i),
                .srcArrayElement = 0,
                .dstSet = curr_copy.descriptor_set,
                .dstBinding = static_cast<std::uint32_t>(i),
                .dstArrayElement = 0,
                .descriptorCount = static_cast<std::uint32_t>(binding.size),
            };
            descriptor_copy.push_back(copy);
        }
    }

    if (!descriptor_copy.empty())
    {
        vkUpdateDescriptorSets(
            m_context->get_device(),
            0,
            nullptr,
            static_cast<std::uint32_t>(descriptor_copy.size()),
            descriptor_copy.data());
    }

    m_dirty = remaining_update_count != 0;
    return !m_dirty;
}

VkDescriptorSet vk_parameter::get_descriptor_set() const noexcept
{
    if (m_copies.size() == 1)
    {
        return m_copies[0].descriptor_set;
    }

    return m_copies[m_context->get_frame_resource_index()].descriptor_set;
}

const std::vector<buffer_allocation>& vk_parameter::get_uniforms() const noexcept
{
    if (m_copies.size() == 1)
    {
        return m_copies[0].uniforms;
    }

    return m_copies[m_context->get_frame_resource_index()].uniforms;
}

void vk_parameter::mark_dirty(std::uint32_t index)
{
    if (m_flags != 0)
    {
        return;
    }

    m_update_counts[index] = m_context->get_frame_resource_count();

    if (!m_dirty)
    {
        m_context->get_parameter_manager()->add_dirty_parameter(this);
        m_dirty = true;
    }
}

vk_parameter_manager::vk_parameter_manager(vk_context* context)
    : m_uniform_allocator(uniform_buffer_size),
      m_context(context)
{
    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = uniform_buffer_size,
        .usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    };

    VmaAllocationCreateInfo create_info = {
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VmaAllocationInfo allocation_info = {};

    vk_check(vmaCreateBuffer(
        m_context->get_vma_allocator(),
        &buffer_info,
        &create_info,
        &m_uniform_buffer,
        &m_uniform_allocation,
        &allocation_info));

    assert(allocation_info.pMappedData);
    m_uniform_pointer = allocation_info.pMappedData;
}

vk_parameter_manager::~vk_parameter_manager()
{
    vmaDestroyBuffer(m_context->get_vma_allocator(), m_uniform_buffer, m_uniform_allocation);
}

void vk_parameter_manager::add_dirty_parameter(vk_parameter* parameter)
{
    std::scoped_lock lock(m_mutex);

    auto& queue = m_update_queues[m_context->get_frame_resource_index() % 2];
    queue.push_back(parameter);
}

void vk_parameter_manager::remove_dirty_parameter(vk_parameter* parameter)
{
    std::scoped_lock lock(m_mutex);

    for (auto& queue : m_update_queues)
    {
        auto iter = std::find(queue.begin(), queue.end(), parameter);
        if (iter != queue.end())
        {
            queue.erase(iter);
        }
    }
}

void vk_parameter_manager::sync_parameter()
{
    auto& next_queue = m_update_queues[m_context->get_frame_resource_index() % 2];
    auto& prev_queue = m_update_queues[(m_context->get_frame_resource_index() + 1) % 2];

    for (vk_parameter* parameter : prev_queue)
    {
        if (!parameter->sync())
        {
            next_queue.push_back(parameter);
        }
    }

    prev_queue.clear();
}

buffer_allocation vk_parameter_manager::allocate_uniform(std::size_t size)
{
    std::size_t alignment =
        m_context->get_physical_device_properties().limits.minUniformBufferOffsetAlignment;
    size = (size + alignment - 1) & ~(alignment - 1);

    std::scoped_lock lock(m_mutex);
    return m_uniform_allocator.allocate(size);
}

void vk_parameter_manager::free_uniform(buffer_allocation allocation)
{
    std::scoped_lock lock(m_mutex);
    m_uniform_allocator.free(allocation);
}
} // namespace violet::vk
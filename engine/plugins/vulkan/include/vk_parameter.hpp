#pragma once

#include "common/allocator.hpp"
#include "vk_common.hpp"
#include <array>
#include <mutex>
#include <vector>

namespace violet::vk
{
class vk_context;
class vk_parameter_layout;

class vk_parameter : public rhi_parameter
{
public:
    vk_parameter(const rhi_parameter_desc& desc, vk_context* context);
    virtual ~vk_parameter();

    void set_constant(std::size_t index, const void* data, std::size_t size, std::size_t offset)
        override;
    void set_uniform(std::size_t index, rhi_buffer* uniform, std::size_t offset) override;
    void set_storage(std::size_t index, rhi_buffer* storage, std::size_t offset) override;
    void set_texture(std::size_t index, rhi_texture* texture, std::size_t offset) override;
    void set_sampler(std::size_t index, rhi_sampler* sampler, std::size_t offset) override;

    bool sync();

    VkDescriptorSet get_descriptor_set() const noexcept;

private:
    struct copy
    {
        std::vector<buffer_allocation> constants;
        VkDescriptorSet descriptor_set;
    };

    const std::vector<buffer_allocation>& get_constans() const noexcept;

    void mark_dirty(std::size_t index);

    vk_parameter_layout* m_layout;

    std::vector<std::uint32_t> m_update_counts;

    std::vector<copy> m_copies;

    rhi_parameter_flags m_flags;

    bool m_dirty{false};

    vk_context* m_context;
};

class vk_buffer;
class vk_parameter_manager
{
public:
    vk_parameter_manager(vk_context* context);
    ~vk_parameter_manager();

    void add_dirty_parameter(vk_parameter* parameter);
    void remove_dirty_parameter(vk_parameter* parameter);

    void sync_parameter();

    buffer_allocation allocate_constant(std::size_t size);
    void free_constant(buffer_allocation allocation);

    void* get_constant_pointer(std::size_t offset = 0)
    {
        return static_cast<std::uint8_t*>(m_constant_pointer) + offset;
    }

    VkBuffer get_constant_buffer() const noexcept
    {
        return m_constant_buffer;
    }

private:
    static constexpr std::size_t constant_buffer_size = 8 * 1024 * 1024;

    std::array<std::vector<vk_parameter*>, 2> m_update_queues;

    VkBuffer m_constant_buffer;
    VmaAllocation m_constant_allocation;
    void* m_constant_pointer{nullptr};

    buffer_allocator m_constant_allocator;

    std::mutex m_mutex;

    vk_context* m_context;
};
} // namespace violet::vk
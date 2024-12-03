#pragma once

#include "common/allocator.hpp"
#include "graphics/gpu_buffer_uploader.hpp"
#include "graphics/material.hpp"
#include <mutex>

namespace violet
{
class material_manager
{
public:
    material_manager(std::size_t material_buffer_size = 8ull * 1024 * 1024);

    render_id register_material(material* material, std::uint32_t& constant_address);
    void unregister_material(render_id material_id);

    bool update();
    void record(rhi_command* command);

    void mark_dirty(material* material);

    rhi_buffer* get_material_buffer() const noexcept
    {
        return m_material_buffer.get();
    }

private:
    struct material_info
    {
        material* material;
        buffer_allocation constant_allocation;
    };

    std::vector<material_info> m_materials;
    index_allocator<render_id> m_material_allocator;

    std::vector<render_id> m_dirty_materials;

    rhi_ptr<rhi_buffer> m_material_buffer;
    buffer_allocator m_material_buffer_allocator;

    gpu_buffer_uploader m_gpu_buffer_uploader;

    std::mutex m_mutex;
};
} // namespace violet
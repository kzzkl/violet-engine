#pragma once

#include "common/allocator.hpp"
#include "graphics/material.hpp"
#include "graphics/resources/persistent_buffer.hpp"
#include <mutex>

namespace violet
{
class gpu_buffer_uploader;
class material_manager
{
public:
    material_manager(std::size_t material_buffer_size = 64ull * 1024);
    ~material_manager();

    render_id add_material(material* material);
    void remove_material(render_id material_id);

    void update(gpu_buffer_uploader* uploader);
    void update_constant(render_id material_id, const void* data, std::size_t size);

    void mark_dirty(render_id material_id);

    std::uint32_t get_material_constant_address(render_id material_id) const
    {
        return static_cast<std::uint32_t>(m_materials[material_id].constant_allocation.offset);
    }

    raw_buffer* get_material_buffer() const noexcept
    {
        return m_material_buffer.get();
    }

private:
    struct material_info
    {
        material* material;

        std::size_t constant_size;
        buffer_allocation constant_allocation;
    };

    std::vector<material_info> m_materials;
    index_allocator<render_id> m_material_allocator;

    std::vector<render_id> m_dirty_materials;

    std::unique_ptr<persistent_buffer> m_material_buffer;

    std::mutex m_mutex;
};
} // namespace violet
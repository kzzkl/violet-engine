#pragma once

#include "graphics/render_graph/rdg_node.hpp"
#include "graphics/render_interface.hpp"
#include <vector>

namespace violet
{
enum rdg_resource_type
{
    RDG_RESOURCE_TEXTURE,
    RDG_RESOURCE_BUFFER
};

class rdg_reference;
class rdg_resource : public rdg_node
{
public:
    virtual ~rdg_resource() = default;

    void reset() noexcept override
    {
        m_references.clear();
        m_external = false;

        rdg_node::reset();
    }

    virtual rdg_resource_type get_type() const noexcept = 0;

    void set_external(bool external) noexcept
    {
        m_external = external;
    }

    bool is_external() const noexcept
    {
        return m_external;
    }

    std::size_t add_reference(rdg_reference* reference)
    {
        std::size_t index = m_references.size();
        m_references.push_back(reference);
        return index;
    }

    const std::vector<rdg_reference*>& get_references() const noexcept
    {
        return m_references;
    }

private:
    std::vector<rdg_reference*> m_references;
    bool m_external;
};

class rdg_texture : public rdg_resource
{
public:
    rdg_resource_type get_type() const noexcept final
    {
        return RDG_RESOURCE_TEXTURE;
    }

    void set_extent(const rhi_texture_extent& extent) noexcept
    {
        m_extent = extent;
    }

    rhi_texture_extent get_extent() const noexcept
    {
        return m_extent;
    }

    void set_format(rhi_format format) noexcept
    {
        m_format = format;
    }

    rhi_format get_format() const noexcept
    {
        return m_format;
    }

    void set_flags(rhi_texture_flags flags) noexcept
    {
        m_flags = flags;
    }

    rhi_texture_flags get_flags() const noexcept
    {
        return m_flags;
    }

    void set_samples(rhi_sample_count samples) noexcept
    {
        m_samples = samples;
    }

    rhi_sample_count get_samples() const noexcept
    {
        return m_samples;
    }

    void set_level_count(std::uint32_t level_count) noexcept
    {
        m_level_count = level_count;
    }

    std::uint32_t get_level_count() const noexcept
    {
        return m_level_count;
    }

    void set_layer_count(std::uint32_t layer_count) noexcept
    {
        m_layer_count = layer_count;
    }

    std::uint32_t get_layer_count() const noexcept
    {
        return m_layer_count;
    }

    void set_initial_layout(rhi_texture_layout layout) noexcept
    {
        m_initial_layout = layout;
    }

    rhi_texture_layout get_initial_layout() const noexcept
    {
        return m_initial_layout;
    }

    void set_final_layout(rhi_texture_layout layout) noexcept
    {
        m_final_layout = layout;
    }

    rhi_texture_layout get_final_layout() const noexcept
    {
        return m_final_layout;
    }

    void set_rhi(rhi_texture* texture) noexcept
    {
        m_texture = texture;
    }

    rhi_texture* get_rhi() const noexcept
    {
        return m_texture;
    }

    void build_texture_barriers();

private:
    rhi_texture* m_texture{nullptr};

    rhi_texture_extent m_extent;
    rhi_format m_format;
    rhi_texture_flags m_flags;

    std::uint32_t m_level_count{1};
    std::uint32_t m_layer_count{1};

    rhi_sample_count m_samples;

    rhi_texture_layout m_initial_layout{RHI_TEXTURE_LAYOUT_UNDEFINED};
    rhi_texture_layout m_final_layout{RHI_TEXTURE_LAYOUT_UNDEFINED};
};

class rdg_buffer : public rdg_resource
{
public:
    rdg_resource_type get_type() const noexcept final
    {
        return RDG_RESOURCE_BUFFER;
    }

    void set_size(std::size_t size) noexcept
    {
        m_size = size;
    }

    std::size_t get_size() const
    {
        return m_size;
    }

    void set_flags(rhi_buffer_flags flags) noexcept
    {
        m_flags = flags;
    }

    rhi_buffer_flags get_flags() const noexcept
    {
        return m_flags;
    }

    void set_rhi(rhi_buffer* buffer) noexcept
    {
        m_buffer = buffer;
    }

    rhi_buffer* get_rhi() const noexcept
    {
        return m_buffer;
    }

protected:
    rhi_buffer* m_buffer{nullptr};

    std::size_t m_size{0};
    rhi_buffer_flags m_flags{0};
};
} // namespace violet
#pragma once

#include "graphics/render_graph/rdg_node.hpp"
#include "graphics/render_interface.hpp"
#include <string>
#include <vector>

namespace violet
{
enum rdg_reference_type
{
    RDG_REFERENCE_NONE,
    RDG_REFERENCE_TEXTURE,
    RDG_REFERENCE_BUFFER,
    RDG_REFERENCE_ATTACHMENT
};

class rdg_pass;
class rdg_resource;
struct rdg_reference
{
    rdg_pass* pass;
    rdg_resource* resource;

    rdg_reference_type type;
    rhi_pipeline_stage_flags stages;
    rhi_access_flags access;

    std::size_t index;

    union
    {
        struct
        {
            rhi_texture_layout layout;
            rhi_attachment_reference_type type;
            rhi_attachment_load_op load_op;
            rhi_attachment_store_op store_op;
        } attachment;

        struct
        {
            rhi_texture_layout layout;
        } texture;
    };

    bool is_first_reference() const noexcept;

    bool is_last_reference() const noexcept;

    rdg_reference* get_prev_reference() const;

    rdg_reference* get_next_reference() const;

    rhi_texture_layout get_texture_layout() const noexcept
    {
        switch (type)
        {
        case RDG_REFERENCE_TEXTURE:
            return texture.layout;
        case RDG_REFERENCE_ATTACHMENT:
            return attachment.layout;
        default:
            return RHI_TEXTURE_LAYOUT_UNDEFINED;
        }
    }
};

enum rdg_resource_type
{
    RDG_RESOURCE_TEXTURE,
    RDG_RESOURCE_BUFFER
};

class rdg_resource : public rdg_node
{
public:
    rdg_resource();
    virtual ~rdg_resource();

    virtual rdg_resource_type get_type() const noexcept = 0;
    virtual bool is_external() const noexcept = 0;

    void add_reference(rdg_reference* reference)
    {
        reference->index = m_references.size();
        m_references.push_back(reference);
    }
    const std::vector<rdg_reference*>& get_references() const noexcept
    {
        return m_references;
    }

private:
    std::vector<rdg_reference*> m_references;
};

class rdg_texture : public rdg_resource
{
public:
    rdg_texture(
        rhi_texture* texture,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    rdg_resource_type get_type() const noexcept final
    {
        return RDG_RESOURCE_TEXTURE;
    }

    bool is_external() const noexcept override
    {
        return true;
    }

    virtual bool is_texture_view() const noexcept
    {
        return false;
    }

    virtual rhi_texture_extent get_extent() const noexcept
    {
        return m_texture->get_extent();
    }

    virtual rhi_format get_format() const noexcept
    {
        return m_texture->get_format();
    }

    virtual rhi_sample_count get_samples() const noexcept
    {
        return m_texture->get_samples();
    }

    rhi_texture* get_rhi() const noexcept
    {
        return m_texture;
    }

    rhi_texture_layout get_initial_layout() const noexcept
    {
        return m_initial_layout;
    }

    rhi_texture_layout get_final_layout() const noexcept
    {
        return m_final_layout;
    }

private:
    rhi_texture* m_texture{nullptr};

    rhi_texture_layout m_initial_layout;
    rhi_texture_layout m_final_layout;

    friend class render_graph;
};

class rdg_inter_texture : public rdg_texture
{
public:
    rdg_inter_texture(
        const rhi_texture_desc& desc,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    virtual bool is_external() const noexcept
    {
        return false;
    }

    rhi_texture_extent get_extent() const noexcept override
    {
        return m_desc.extent;
    }

    rhi_format get_format() const noexcept override
    {
        return m_desc.format;
    }

    rhi_sample_count get_samples() const noexcept override
    {
        return m_desc.samples;
    }

    const rhi_texture_desc& get_desc() const noexcept
    {
        return m_desc;
    }

private:
    rhi_texture_desc m_desc;
};

class rdg_texture_view : public rdg_texture
{
public:
    rdg_texture_view(
        rhi_texture* texture,
        std::uint32_t level,
        std::uint32_t layer,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    rdg_texture_view(
        rdg_texture* texture,
        std::uint32_t level,
        std::uint32_t layer,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    bool is_external() const noexcept override
    {
        return m_rhi_texture ? true : m_rdg_texture->is_external();
    }

    bool is_texture_view() const noexcept final
    {
        return true;
    }

    rhi_texture_extent get_extent() const noexcept override
    {
        return m_rhi_texture ? m_rhi_texture->get_extent() : m_rdg_texture->get_extent();
    }

    rhi_format get_format() const noexcept override
    {
        return m_rhi_texture ? m_rhi_texture->get_format() : m_rdg_texture->get_format();
    }

    rhi_sample_count get_samples() const noexcept override
    {
        return m_rhi_texture ? m_rhi_texture->get_samples() : m_rdg_texture->get_samples();
    }

private:
    rhi_texture* m_rhi_texture{nullptr};
    rdg_texture* m_rdg_texture{nullptr};

    std::uint32_t m_level{0};
    std::uint32_t m_layer{0};

    friend class render_graph;
};

class rdg_buffer : public rdg_resource
{
public:
    explicit rdg_buffer(rhi_buffer* buffer = nullptr);

    virtual rdg_resource_type get_type() const noexcept override final
    {
        return RDG_RESOURCE_TEXTURE;
    }

    virtual bool is_external() const noexcept override
    {
        return true;
    }

private:
    rhi_buffer* m_buffer;

    friend class render_graph;
};
} // namespace violet
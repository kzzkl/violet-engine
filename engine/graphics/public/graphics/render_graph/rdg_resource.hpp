#pragma once

#include "graphics/render_graph/rdg_node.hpp"
#include "graphics/render_interface.hpp"
#include <string>
#include <vector>

namespace violet
{

enum rdg_reference_type
{
    RDG_REFERENCE_TYPE_NONE,
    RDG_REFERENCE_TYPE_TEXTURE,
    RDG_REFERENCE_TYPE_BUFFER,
    RDG_REFERENCE_TYPE_ATTACHMENT
};

class rdg_pass;
class rdg_resource;
struct rdg_reference
{
    rdg_pass* pass;
    rdg_resource* resource;

    rdg_reference_type type;
    rhi_access_flags access;

    union {
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

    rhi_texture_layout get_texture_layout() const noexcept
    {
        switch (type)
        {
        case RDG_REFERENCE_TYPE_TEXTURE:
            return texture.layout;
        case RDG_REFERENCE_TYPE_ATTACHMENT:
            return attachment.layout;
        default:
            return RHI_TEXTURE_LAYOUT_UNDEFINED;
        }
    }
};

enum rdg_resource_type
{
    RDG_RESOURCE_TYPE_TEXTURE,
    RDG_RESOURCE_TYPE_BUFFER
};

class rdg_resource : public rdg_node
{
public:
    rdg_resource(bool external);
    virtual ~rdg_resource();

    virtual rdg_resource_type get_type() const noexcept = 0;

    void add_reference(rdg_reference* reference) { m_references.push_back(reference); }
    const std::vector<rdg_reference*>& get_references() const noexcept { return m_references; }

    bool is_first_pass(rdg_pass* pass)
    {
        return !m_references.empty() && m_references[0]->pass == pass;
    }
    bool is_last_pass(rdg_pass* pass)
    {
        return !m_references.empty() && m_references.back()->pass == pass;
    }
    bool is_external() const noexcept { return m_external; }

private:
    bool m_external;
    std::vector<rdg_reference*> m_references;
};

class rdg_texture : public rdg_resource
{
public:
    rdg_texture(
        rhi_texture* texture,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);
    rdg_texture(
        const rhi_texture_desc& desc,
        rhi_texture_layout initial_layout,
        rhi_texture_layout final_layout);

    virtual rdg_resource_type get_type() const noexcept override final
    {
        return RDG_RESOURCE_TYPE_TEXTURE;
    }

    rhi_format get_format() const noexcept { return m_desc.format; }
    rhi_sample_count get_samples() const noexcept { return m_desc.samples; }

    rhi_texture* get_rhi() const noexcept { return m_texture; }

    rhi_texture_layout get_initial_layout() const noexcept { return m_initial_layout; }
    rhi_texture_layout get_final_layout() const noexcept { return m_final_layout; }

private:
    rhi_texture_desc m_desc;
    rhi_texture* m_texture{nullptr};

    rhi_texture_layout m_initial_layout;
    rhi_texture_layout m_final_layout;

    friend class render_graph;
};

class rdg_buffer : public rdg_resource
{
public:
    rdg_buffer(rhi_buffer* buffer = nullptr);

    virtual rdg_resource_type get_type() const noexcept override final
    {
        return RDG_RESOURCE_TYPE_TEXTURE;
    }

private:
    rhi_buffer* m_buffer;

    friend class render_graph;
};
} // namespace violet
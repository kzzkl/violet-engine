#pragma once

#include "graphics/render_interface.hpp"
#include <string>

namespace violet
{
enum rdg_resource_type
{
    RDG_RESOURCE_TYPE_TEXTURE,
    RDG_RESOURCE_TYPE_BUFFER
};

class rdg_resource
{
public:
    rdg_resource();
    virtual ~rdg_resource();

    const std::string& get_name() const noexcept { return m_name; }
    std::size_t get_index() const noexcept { return m_index; }
    virtual rdg_resource_type get_type() const noexcept = 0;

    bool is_external() const noexcept { return m_external; }

private:
    friend class render_graph;
    std::string m_name;
    std::size_t m_index;
    bool m_external;
};

class rdg_texture : public rdg_resource
{
public:
    rdg_texture();

    virtual rdg_resource_type get_type() const noexcept override final
    {
        return RDG_RESOURCE_TYPE_TEXTURE;
    }

    void set_format(rhi_format format) noexcept { m_format = format; }
    rhi_format get_format() const noexcept { return m_format; }

    void set_samples(rhi_sample_count samples) noexcept { m_samples = samples; }
    rhi_sample_count get_samples() const noexcept { return m_samples; }

private:
    rhi_format m_format;
    rhi_sample_count m_samples;
};

class rdg_buffer : public rdg_resource
{
public:
    using rdg_resource::rdg_resource;

    virtual rdg_resource_type get_type() const noexcept override final
    {
        return RDG_RESOURCE_TYPE_TEXTURE;
    }
};
} // namespace violet
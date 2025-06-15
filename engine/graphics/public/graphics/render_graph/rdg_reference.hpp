#pragma once

#include "graphics/render_graph/rdg_resource.hpp"

namespace violet
{
class rdg_pass;

enum rdg_reference_type
{
    RDG_REFERENCE_TEXTURE,
    RDG_REFERENCE_TEXTURE_SRV,
    RDG_REFERENCE_TEXTURE_UAV,
    RDG_REFERENCE_TEXTURE_RTV,
    RDG_REFERENCE_TEXTURE_DSV,
    RDG_REFERENCE_BUFFER,
    RDG_REFERENCE_BUFFER_SRV,
    RDG_REFERENCE_BUFFER_UAV,
};

class rdg_reference
{
public:
    rdg_reference_type type;
    rdg_pass* pass;
    rdg_resource* resource;
    rhi_pipeline_stage_flags stages;
    rhi_access_flags access;
    std::size_t index;

    union
    {
        struct
        {
            rhi_texture_layout layout;

            std::uint32_t level;
            std::uint32_t level_count;
            std::uint32_t layer;
            std::uint32_t layer_count;

            union
            {
                struct
                {
                    rhi_texture_dimension dimension;
                    rhi_texture_srv* rhi;
                } srv;

                struct
                {
                    rhi_texture_dimension dimension;
                    rhi_texture_uav* rhi;
                } uav;

                struct
                {
                    rhi_attachment_load_op load_op;
                    rhi_attachment_store_op store_op;

                    rhi_clear_value clear_value;

                    rhi_texture_rtv* rhi;
                } rtv;

                struct
                {
                    rhi_attachment_load_op load_op;
                    rhi_attachment_store_op store_op;

                    rhi_clear_value clear_value;

                    rhi_texture_dsv* rhi;
                } dsv;
            };
        } texture;

        struct
        {
            std::size_t offset;
            std::size_t size;

            union
            {
                struct
                {
                    rhi_buffer_srv* rhi;
                    rhi_format texel_format;
                } srv;

                struct
                {
                    rhi_buffer_uav* rhi;
                    rhi_format texel_format;
                } uav;
            };
        } buffer;
    };
};

class rdg_reference_wrapper
{
public:
    explicit rdg_reference_wrapper(const rdg_reference* data = nullptr) noexcept
        : m_data(data)
    {
    }

    operator bool() const noexcept
    {
        return m_data != nullptr;
    }

    rdg_pass* get_pass() const noexcept
    {
        return m_data->pass;
    }

    rdg_resource* get_resource() const noexcept
    {
        return m_data->resource;
    }

    rhi_pipeline_stage_flags get_stages() const noexcept
    {
        return m_data->stages;
    }

    rhi_access_flags get_access() const noexcept
    {
        return m_data->access;
    }

    bool is_first_reference() const noexcept;
    bool is_last_reference() const noexcept;

    rdg_reference* get_prev_reference() const;
    rdg_reference* get_next_reference() const;

    void reset() noexcept
    {
        m_data = nullptr;
    }

protected:
    const rdg_reference& get_data() const noexcept
    {
        return *m_data;
    }

private:
    const rdg_reference* m_data;
};

class rdg_texture_ref : public rdg_reference_wrapper
{
public:
    explicit rdg_texture_ref(const rdg_reference* data = nullptr) noexcept
        : rdg_reference_wrapper(data)
    {
    }

    rhi_texture_layout get_layout() const noexcept
    {
        return get_data().texture.layout;
    }

    std::uint32_t get_level() const noexcept
    {
        return get_data().texture.level;
    }

    std::uint32_t get_level_count() const noexcept
    {
        return get_data().texture.level_count;
    }

    std::uint32_t get_layer() const noexcept
    {
        return get_data().texture.layer;
    }

    std::uint32_t get_layer_count() const noexcept
    {
        return get_data().texture.layer_count;
    }

    rhi_texture_extent get_extent() const noexcept
    {
        rhi_texture_extent extent = get_texture()->get_extent();
        extent.width = std::max(extent.width >> get_level(), 1u);
        extent.height = std::max(extent.height >> get_level(), 1u);
        return extent;
    }

    rdg_texture* get_texture() const noexcept
    {
        return static_cast<rdg_texture*>(get_data().resource);
    }

    rhi_texture* get_rhi() const noexcept
    {
        return static_cast<rdg_texture*>(get_resource())->get_rhi();
    }
};

class rdg_texture_srv : public rdg_texture_ref
{
public:
    explicit rdg_texture_srv(const rdg_reference* data = nullptr) noexcept
        : rdg_texture_ref(data)
    {
    }

    std::uint32_t get_bindless() const noexcept
    {
        return get_rhi()->get_bindless();
    }

    rhi_texture_srv* get_rhi() const noexcept
    {
        return get_data().texture.srv.rhi;
    }
};

class rdg_texture_uav : public rdg_texture_ref
{
public:
    explicit rdg_texture_uav(const rdg_reference* data = nullptr) noexcept
        : rdg_texture_ref(data)
    {
    }

    std::uint32_t get_bindless() const noexcept
    {
        return get_rhi()->get_bindless();
    }

    rhi_texture_uav* get_rhi() const noexcept
    {
        return get_data().texture.uav.rhi;
    }
};

class rdg_texture_rtv : public rdg_texture_ref
{
public:
    explicit rdg_texture_rtv(const rdg_reference* data = nullptr) noexcept
        : rdg_texture_ref(data)
    {
    }

    rhi_attachment_load_op get_load_op() const noexcept
    {
        return get_data().texture.rtv.load_op;
    }

    rhi_attachment_store_op get_store_op() const noexcept
    {
        return get_data().texture.rtv.store_op;
    }
};

class rdg_texture_dsv : public rdg_texture_ref
{
public:
    explicit rdg_texture_dsv(const rdg_reference* data = nullptr) noexcept
        : rdg_texture_ref(data)
    {
    }

    rhi_attachment_load_op get_load_op() const noexcept
    {
        return get_data().texture.dsv.load_op;
    }

    rhi_attachment_store_op get_store_op() const noexcept
    {
        return get_data().texture.dsv.store_op;
    }
};

class rdg_buffer_ref : public rdg_reference_wrapper
{
public:
    explicit rdg_buffer_ref(const rdg_reference* data = nullptr) noexcept
        : rdg_reference_wrapper(data)
    {
    }

    std::size_t get_offset() const noexcept
    {
        return get_data().buffer.offset;
    }

    std::size_t get_size() const noexcept
    {
        return get_data().buffer.size;
    }

    rhi_buffer* get_rhi() const noexcept
    {
        return static_cast<rdg_buffer*>(get_resource())->get_rhi();
    }
};

class rdg_buffer_srv : public rdg_buffer_ref
{
public:
    explicit rdg_buffer_srv(const rdg_reference* data = nullptr) noexcept
        : rdg_buffer_ref(data)
    {
    }

    std::uint32_t get_bindless() const noexcept
    {
        return get_rhi()->get_bindless();
    }

    rhi_buffer_srv* get_rhi() const noexcept
    {
        return get_data().buffer.srv.rhi;
    }
};

class rdg_buffer_uav : public rdg_buffer_ref
{
public:
    explicit rdg_buffer_uav(const rdg_reference* data = nullptr) noexcept
        : rdg_buffer_ref(data)
    {
    }

    std::uint32_t get_bindless() const noexcept
    {
        return get_rhi()->get_bindless();
    }

    rhi_buffer_uav* get_rhi() const noexcept
    {
        return get_data().buffer.uav.rhi;
    }
};
} // namespace violet
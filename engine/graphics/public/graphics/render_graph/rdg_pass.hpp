#pragma once

#include "graphics/render_device.hpp"
#include "graphics/render_graph/rdg_context.hpp"
#include "graphics/render_graph/rdg_resource.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace violet
{
enum rdg_pass_reference_type
{
    RDG_PASS_REFERENCE_TYPE_NONE,
    RDG_PASS_REFERENCE_TYPE_TEXTURE,
    RDG_PASS_REFERENCE_TYPE_BUFFER,
    RDG_PASS_REFERENCE_TYPE_ATTACHMENT
};

struct rdg_pass_reference
{
    rdg_resource* resource;

    rdg_pass_reference_type type;
    rhi_access_flags access;

    union {
        struct
        {
            rhi_texture_layout layout;
            rhi_texture_layout next_layout;
            rhi_attachment_load_op load_op;
            rhi_attachment_store_op store_op;
            rhi_attachment_reference_type type;
        } attachment;

        struct
        {
            rhi_texture_layout layout;
            rhi_texture_layout next_layout;
        } texture;
    };
};

enum rdg_pass_type
{
    RDG_PASS_TYPE_RENDER,
    RDG_PASS_TYPE_COMPUTE,
    RDG_PASS_TYPE_OTHER
};

enum rdg_pass_parameter_flag
{
    RDG_PASS_PARAMETER_FLAG_NONE,
    RDG_PASS_PARAMETER_FLAG_MATERIAL,
    RDG_PASS_PARAMETER_FLAG_PASS
};
using rdg_pass_parameter_flags = std::uint32_t;

class rdg_pass
{
public:
    rdg_pass();
    virtual ~rdg_pass();

    rdg_pass_reference* add_texture(
        std::size_t index,
        rhi_access_flags access,
        rhi_texture_layout layout);
    rdg_pass_reference* add_buffer(std::size_t index, rhi_access_flags access);
    rdg_pass_reference* get_reference(std::size_t index);

    std::vector<rdg_pass_reference*> get_references(rhi_access_flags access) const;
    std::vector<rdg_pass_reference*> get_references(rdg_pass_reference_type type) const;
    std::vector<rdg_pass_reference*> get_references() const;

    const std::string& get_name() const noexcept { return m_name; }
    std::size_t get_index() const noexcept { return m_index; }
    virtual rdg_pass_type get_type() const noexcept { return RDG_PASS_TYPE_OTHER; }

    virtual void compile(render_device* device) {}
    virtual void execute(rhi_command* command, rdg_context* context) = 0;

    void set_input_layout(const std::vector<std::pair<std::string, rhi_format>>& layout)
    {
        m_input_layout = layout;
    }

    const std::vector<std::pair<std::string, rhi_format>>& get_input_layout() const noexcept
    {
        return m_input_layout;
    }

    void set_parameter_layout(
        const std::vector<std::pair<rhi_parameter_desc, rdg_pass_parameter_flags>>& layout)
    {
        m_parameter_layout = layout;
    }

    std::vector<rhi_parameter_desc> get_parameter_layout(
        rdg_pass_parameter_flags flags = RDG_PASS_PARAMETER_FLAG_NONE) const;

protected:
    rdg_pass_reference* add_reference(std::size_t index);

private:
    friend class render_graph;
    std::string m_name;
    std::size_t m_index;

    std::vector<std::unique_ptr<rdg_pass_reference>> m_references;

    std::vector<std::pair<std::string, rhi_format>> m_input_layout;
    std::vector<std::pair<rhi_parameter_desc, rdg_pass_parameter_flags>> m_parameter_layout;
};

class rdg_render_pass : public rdg_pass
{
public:
    rdg_render_pass();

    rdg_pass_reference* add_input(std::size_t index, rhi_texture_layout layout);
    rdg_pass_reference* add_color(
        std::size_t index,
        rhi_texture_layout layout,
        const rhi_attachment_blend_desc& blend = {});
    rdg_pass_reference* add_depth_stencil(std::size_t index, rhi_texture_layout layout);

    void set_shader(std::string_view vertex_shader, std::string_view fragment_shader)
    {
        m_vertex_shader = vertex_shader;
        m_fragment_shader = fragment_shader;
    }
    void set_primitive_topology(rhi_primitive_topology topology) noexcept;

    void set_depth_stencil(const rhi_depth_stencil_desc& depth_stencil) noexcept;
    void set_cull_mode(rhi_cull_mode mode) noexcept;

    void set_render_pass(rhi_render_pass* render_pass, std::uint32_t subpass_index) noexcept;

    virtual rdg_pass_type get_type() const noexcept final { return RDG_PASS_TYPE_RENDER; }

    virtual void compile(render_device* device) override;

protected:
    rhi_render_pipeline* get_pipeline() const noexcept { return m_pipeline.get(); }

private:
    std::string m_vertex_shader;
    std::string m_fragment_shader;
    std::unique_ptr<rhi_render_pipeline_desc> m_desc;

    rhi_ptr<rhi_render_pipeline> m_pipeline;
};

class rdg_compute_pass : public rdg_pass
{
public:
    rdg_compute_pass();

    void set_shader(std::string_view compute_shader) { m_compute_shader = compute_shader; }

    virtual rdg_pass_type get_type() const noexcept final { return RDG_PASS_TYPE_COMPUTE; }

    virtual void compile(render_device* device) override;

protected:
    rhi_compute_pipeline* get_pipeline() const noexcept { return m_pipeline.get(); }

private:
    std::string m_compute_shader;
    std::unique_ptr<rhi_compute_pipeline_desc> m_desc;

    rhi_ptr<rhi_compute_pipeline> m_pipeline;
};
} // namespace violet
#pragma once

#include "graphics/render_graph/render_context.hpp"
#include "graphics/render_graph/resource.hpp"
#include "graphics/renderer.hpp"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace violet
{
enum pass_access_flag : std::uint32_t
{
    PASS_ACCESS_FLAG_READ = 1 << 0,
    PASS_ACCESS_FLAG_WRITE = 1 << 1
};
using pass_access_flags = std::uint32_t;

enum pass_reference_type
{
    PASS_REFERENCE_TYPE_TEXTURE,
    PASS_REFERENCE_TYPE_BUFFER,
    PASS_REFERENCE_TYPE_ATTACHMENT
};

struct pass_reference
{
    std::string name;
    resource* resource;

    pass_reference_type type;
    pass_access_flags access;

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

enum pass_type
{
    PASS_TYPE_RENDER,
    PASS_TYPE_COMPUTE,
    PASS_TYPE_OTHER
};

class pass
{
public:
    pass();
    virtual ~pass();

    pass_reference* add_texture(
        std::string_view name,
        pass_access_flags access,
        rhi_texture_layout layout);
    pass_reference* add_buffer(std::string_view name, pass_access_flags access);
    pass_reference* get_reference(std::string_view name);

    std::vector<pass_reference*> get_references(pass_access_flags access) const;
    std::vector<pass_reference*> get_references(pass_reference_type type) const;

    virtual std::vector<rhi_parameter_desc> get_parameter_layout() const { return {}; };

    const std::string& get_name() const noexcept { return m_name; }
    virtual pass_type get_type() const noexcept { return PASS_TYPE_OTHER; }

    virtual void compile(renderer* renderer) {}
    virtual void execute(rhi_render_command* command, render_context* context) {}

protected:
    pass_reference* add_reference();

private:
    friend class render_graph;
    std::string m_name;

    std::vector<std::unique_ptr<pass_reference>> m_references;
};

class render_pass : public pass
{
public:
    render_pass();

    pass_reference* add_input(std::string_view name, rhi_texture_layout layout);
    pass_reference* add_color(
        std::string_view name,
        rhi_texture_layout layout,
        const rhi_attachment_blend_desc& blend = {});
    pass_reference* add_depth_stencil(std::string_view name, rhi_texture_layout layout);

    void set_vertex_shader(std::string_view shader) { m_vertex_shader = shader; }
    const std::string& get_vertex_shader() const noexcept { return m_vertex_shader; }

    void set_fragment_shader(std::string_view shader) { m_fragment_shader = shader; }
    const std::string& get_fragment_shader() const noexcept { return m_fragment_shader; }

    void set_primitive_topology(rhi_primitive_topology topology) noexcept;

    const std::vector<std::string>& get_vertex_attribute_layout() const noexcept;

    void set_render_pass(rhi_render_pass* render_pass, std::uint32_t subpass_index) noexcept;

    virtual pass_type get_type() const noexcept final { return PASS_TYPE_RENDER; }

    virtual void compile(renderer* renderer) override;
    virtual void execute(rhi_render_command* command, render_context* context) {}

protected:
    rhi_render_pipeline* get_pipeline() const noexcept { return m_pipeline.get(); }

private:
    std::string m_vertex_shader;
    std::string m_fragment_shader;
    std::unique_ptr<rhi_render_pipeline_desc> m_desc;

    std::vector<std::string> m_attributes;

    rhi_ptr<rhi_render_pipeline> m_pipeline;
};

struct render_mesh
{
    rhi_parameter* material;
    rhi_parameter* transform;

    std::size_t vertex_start;
    std::size_t vertex_count;
    std::size_t index_start;
    std::size_t index_count;

    std::vector<rhi_buffer*> vertex_buffers;
    rhi_buffer* index_buffer;
};

class mesh_pass : public render_pass
{
public:
    mesh_pass();

    virtual rhi_parameter_desc get_material_parameter_desc() const noexcept { return {}; }

    void add_mesh(const render_mesh* mesh) { m_meshes.push_back(mesh); }
    void clear_mesh() noexcept { m_meshes.clear(); }

protected:
    const std::vector<const render_mesh*>& get_meshes() const noexcept { return m_meshes; }

private:
    std::vector<const render_mesh*> m_meshes;
};
} // namespace violet
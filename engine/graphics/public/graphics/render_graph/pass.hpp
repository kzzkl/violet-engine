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
    PASS_REFERENCE_TYPE_ATTACHMENT,
    PASS_REFERENCE_TYPE_TEXTURE,
    PASS_REFERENCE_TYPE_BUFFER
};

struct pass_reference
{
    pass_reference_type type;
    pass_access_flags access;

    struct
    {
        rhi_texture_layout layout;
        rhi_attachment_load_op load_op;
        rhi_attachment_store_op store_op;
        rhi_attachment_reference_type type;
    } attachment;

    std::string name;
    resource* resource;
};

enum pass_flag : std::uint32_t
{
    PASS_FLAG_NONE = 0,
    PASS_FLAG_RENDER = 1 << 0,
    PASS_FLAG_MESH = 1 << 1,
    PASS_FLAG_COMPUTE = 1 << 2
};
using pass_flags = std::uint32_t;

class pass
{
public:
    pass();
    virtual ~pass();

    void add_texture(std::string_view name, pass_access_flags access);
    void add_buffer(std::string_view name, pass_access_flags access);
    pass_reference* get_reference(std::string_view name);

    std::vector<pass_reference*> get_references(pass_access_flags access) const;
    std::vector<pass_reference*> get_references(pass_reference_type type) const;

    const std::string& get_name() const noexcept { return m_name; }
    pass_flags get_flags() const noexcept { return m_flags; }

    virtual void compile(renderer* renderer){};
    virtual void execute(rhi_render_command* command, render_context* context) = 0;

protected:
    void add_reference(const pass_reference& reference);

private:
    friend class render_graph;
    std::string m_name;
    pass_flags m_flags;

    std::vector<std::unique_ptr<pass_reference>> m_references;
};

class render_pass : public pass
{
public:
    render_pass();

    void add_input(std::string_view name, rhi_texture_layout layout);
    void add_color(
        std::string_view name,
        rhi_texture_layout layout,
        const rhi_attachment_blend_desc& blend = {});
    void add_depth_stencil(std::string_view name, rhi_texture_layout layout);

    void set_vertex_shader(std::string_view shader) { m_vertex_shader = shader; }
    const std::string& get_vertex_shader() const noexcept { return m_vertex_shader; }

    void set_fragment_shader(std::string_view shader) { m_fragment_shader = shader; }
    const std::string& get_fragment_shader() const noexcept { return m_fragment_shader; }

    void set_render_pass(rhi_render_pass* render_pass, std::uint32_t subpass_index) noexcept
    {
        m_render_pass = render_pass;
        m_subpass_index = subpass_index;
    }

    virtual void compile(renderer* renderer) override;
    virtual void execute(rhi_render_command* command, render_context* context) = 0;

private:
    std::string m_vertex_shader;
    std::string m_fragment_shader;

    std::vector<rhi_attachment_blend_desc> m_blend;

    rhi_render_pass* m_render_pass;
    std::uint32_t m_subpass_index;

    rhi_ptr<rhi_render_pipeline> m_pipeline;
};

struct render_mesh
{
    rhi_parameter* material;
    rhi_parameter* transform;
};

class mesh_pass : public render_pass
{
public:
    mesh_pass();

    void set_material_parameter(rhi_parameter_layout* layout);
    rhi_parameter_layout* get_material_parameter();

    void add_mesh(const render_mesh& mesh) { m_meshes.push_back(mesh); }
    void clear_mesh() noexcept { m_meshes.clear(); }

protected:
    const std::vector<render_mesh>& get_meshes() const noexcept { return m_meshes; }

private:
    std::vector<render_mesh> m_meshes;
};
} // namespace violet
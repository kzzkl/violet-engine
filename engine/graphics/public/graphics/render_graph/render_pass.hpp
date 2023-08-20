#pragma once

#include "graphics/render_graph/render_node.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include "graphics/render_graph/render_resource.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace violet
{
class render_attachment
{
public:
    render_attachment(std::size_t index, render_resource* resource);

    void set_load_op(rhi_attachment_load_op op);
    void set_store_op(rhi_attachment_store_op op);
    void set_stencil_load_op(rhi_attachment_load_op op);
    void set_stencil_store_op(rhi_attachment_store_op op);

    void set_initial_state(rhi_resource_state state);
    void set_final_state(rhi_resource_state state);

    rhi_attachment_desc get_desc() const noexcept { return m_desc; }
    std::size_t get_index() const noexcept { return m_index; }

    rhi_resource* get_resource() const noexcept { return m_resource->get_resource(); }

private:
    rhi_attachment_desc m_desc;
    std::size_t m_index;

    render_resource* m_resource;
};

class render_subpass : public render_node
{
public:
    render_subpass(
        std::string_view name,
        rhi_context* rhi,
        render_pass* render_pass,
        std::size_t index);

    void add_reference(
        render_attachment* attachment,
        rhi_attachment_reference_type type,
        rhi_resource_state state,
        render_attachment* resolve = nullptr);

    render_pipeline* add_pipeline(std::string_view name);

    bool compile();
    void execute(rhi_render_command* command);

    rhi_render_subpass_desc get_desc() const noexcept { return m_desc; }

private:
    std::vector<render_attachment*> m_references;
    rhi_render_subpass_desc m_desc;

    render_pass* m_render_pass;
    std::size_t m_index;

    std::vector<std::unique_ptr<render_pipeline>> m_pipelines;
};

class render_pass : public render_node
{
public:
    render_pass(std::string_view name, rhi_context* rhi);
    virtual ~render_pass();

    render_attachment* add_attachment(std::string_view name, render_resource* resource);
    render_subpass* add_subpass(std::string_view name);

    bool compile();
    void execute(rhi_render_command* command);

    rhi_render_pass* get_interface() const noexcept { return m_interface; }

private:
    void update_framebuffer_cache();

    std::vector<std::unique_ptr<render_attachment>> m_attachments;
    std::vector<std::unique_ptr<render_subpass>> m_subpasses;

    rhi_framebuffer* m_framebuffer;
    std::unordered_map<std::size_t, rhi_framebuffer*> m_framebuffer_cache;

    rhi_render_pass* m_interface;
};
} // namespace violet
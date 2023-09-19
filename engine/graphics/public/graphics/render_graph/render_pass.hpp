#pragma once

#include "graphics/render_graph/render_node.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

namespace violet
{
class render_attachment
{
public:
    render_attachment(std::size_t index) noexcept;

    void set_format(rhi_resource_format format) noexcept;

    void set_load_op(rhi_attachment_load_op op) noexcept;
    void set_store_op(rhi_attachment_store_op op) noexcept;
    void set_stencil_load_op(rhi_attachment_load_op op) noexcept;
    void set_stencil_store_op(rhi_attachment_store_op op) noexcept;

    void set_initial_state(rhi_resource_state state) noexcept;
    void set_final_state(rhi_resource_state state) noexcept;

    rhi_attachment_desc get_desc() const noexcept { return m_desc; }
    std::size_t get_index() const noexcept { return m_index; }

private:
    rhi_attachment_desc m_desc;
    std::size_t m_index;
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
        rhi_resource_state state);
    void add_reference(
        render_attachment* attachment,
        rhi_attachment_reference_type type,
        rhi_resource_state state,
        render_attachment* resolve);

    render_pipeline* add_pipeline(std::string_view name);

    bool compile();
    void execute(rhi_render_command* command, rhi_pipeline_parameter* camera_parameter);

    rhi_render_subpass_desc get_desc() const noexcept { return m_desc; }
    std::size_t get_index() const noexcept { return m_index; }

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
    render_pass(const render_pass&) = delete;
    virtual ~render_pass();

    render_attachment* add_attachment(std::string_view name);
    render_subpass* add_subpass(std::string_view name);

    void add_dependency(
        std::size_t source_index,
        rhi_pipeline_stage_flags source_stage,
        rhi_access_flags source_access,
        std::size_t target_index,
        rhi_pipeline_stage_flags target_stage,
        rhi_access_flags target_access);

    void add_camera(
        rhi_scissor_rect scissor,
        rhi_viewport viewport,
        rhi_pipeline_parameter* parameter,
        rhi_framebuffer* framebuffer);

    bool compile();
    void execute(rhi_render_command* command);

    rhi_render_pass* get_interface() const noexcept { return m_interface; }
    std::size_t get_attachment_count() const noexcept { return m_attachments.size(); }

    render_pass& operator=(const render_pass&) = delete;

private:
    struct render_camera
    {
        rhi_scissor_rect scissor;
        rhi_viewport viewport;
        rhi_pipeline_parameter* parameter;
        rhi_framebuffer* framebuffer;
    };

    std::vector<std::unique_ptr<render_attachment>> m_attachments;
    std::vector<std::unique_ptr<render_subpass>> m_subpasses;
    std::vector<rhi_render_subpass_dependency_desc> m_dependencies;

    std::vector<render_camera> m_cameras;

    rhi_render_pass* m_interface;
};
} // namespace violet
#pragma once

#include "graphics/render_graph/pass.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <cassert>
#include <map>
#include <memory>
#include <vector>

namespace violet
{
class render_attachment
{
public:
    render_attachment(render_resource* resource, std::size_t index);

    void set_initial_layout(rhi_image_layout layout) noexcept { m_desc.initial_layout = layout; }
    void set_final_layout(rhi_image_layout layout) noexcept { m_desc.final_layout = layout; }

    void set_load_op(rhi_attachment_load_op op) noexcept { m_desc.load_op = op; }
    void set_store_op(rhi_attachment_store_op op) noexcept { m_desc.store_op = op; }
    void set_stencil_load_op(rhi_attachment_load_op op) noexcept { m_desc.stencil_load_op = op; }
    void set_stencil_store_op(rhi_attachment_store_op op) noexcept { m_desc.stencil_store_op = op; }

    std::size_t get_index() const noexcept { return m_index; }
    const rhi_attachment_desc& get_desc() const noexcept { return m_desc; }

private:
    std::size_t m_index;
    rhi_attachment_desc m_desc;
};

class render_pass;
class render_subpass
{
public:
    render_subpass(render_pass* render_pass, std::size_t index);

    void add_reference(
        render_attachment* attachment,
        rhi_attachment_reference_type type,
        rhi_image_layout layout);
    void add_reference(
        render_attachment* attachment,
        rhi_attachment_reference_type type,
        rhi_image_layout layout,
        render_attachment* resolve);

    std::size_t get_index() const noexcept { return m_index; }
    const rhi_render_subpass_desc& get_desc() const noexcept { return m_desc; }

private:
    render_pass* m_render_pass;

    std::size_t m_index;
    rhi_render_subpass_desc m_desc;
};

class render_pass : public pass
{
public:
    render_pass(renderer* renderer, setup_context& context);
    virtual ~render_pass();

    render_attachment* add_attachment(render_resource* resource);

    render_subpass* add_subpass();

    render_pipeline* add_pipeline(render_subpass* subpass);

    void add_dependency(
        render_subpass* src,
        rhi_pipeline_stage_flags src_stage,
        rhi_access_flags src_access,
        render_subpass* dst,
        rhi_pipeline_stage_flags dst_stage,
        rhi_access_flags dst_access);

    virtual bool compile(compile_context& context) override;
    virtual void execute(execute_context& context) override;

    rhi_render_pass* get_interface() const noexcept { return m_interface.get(); }
    std::size_t get_attachment_count() const noexcept { return m_attachments.size(); }

protected:
    rhi_framebuffer* get_framebuffer(const std::vector<render_resource*>& attachments);

private:
    std::vector<std::unique_ptr<render_attachment>> m_attachments;
    std::vector<rhi_render_subpass_dependency_desc> m_dependencies;

    std::vector<std::unique_ptr<render_subpass>> m_subpasses;
    std::vector<std::unique_ptr<render_pipeline>> m_pipelines;

    rhi_ptr<rhi_framebuffer> m_temporary_framebuffer;
    std::unordered_map<std::size_t, rhi_ptr<rhi_framebuffer>> m_framebuffer_cache;

    rhi_ptr<rhi_render_pass> m_interface;
    renderer* m_renderer;
};
} // namespace violet
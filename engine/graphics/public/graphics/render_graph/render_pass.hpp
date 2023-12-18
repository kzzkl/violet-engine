#pragma once

#include "graphics/render_graph/render_pipeline.hpp"
#include <cassert>
#include <memory>
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

class render_pass;
class render_subpass : public render_node
{
public:
    render_subpass(render_pass* render_pass, std::size_t index);

    void add_reference(
        render_attachment* attachment,
        rhi_attachment_reference_type type,
        rhi_resource_state state);
    void add_reference(
        render_attachment* attachment,
        rhi_attachment_reference_type type,
        rhi_resource_state state,
        render_attachment* resolve);

    template <typename T, typename... Args>
    T* add_pipeline(std::string_view name, Args&&... args)
    {
        assert(m_pipeline_map.find(name.data()) == m_pipeline_map.end());

        auto pipeline = std::make_unique<T>(std::forward<Args>(args)...);
        T* result = pipeline.get();

        m_pipelines.push_back(std::move(pipeline));
        m_pipeline_map[name.data()] = m_pipelines.back().get();
        return result;
    }
    render_pipeline* get_pipeline(std::string_view name) const;

    virtual bool compile(compile_context& context) override;
    virtual void execute(execute_context& context) override;

    rhi_render_subpass_desc get_desc() const noexcept { return m_desc; }
    std::size_t get_index() const noexcept { return m_index; }

private:
    std::vector<render_attachment*> m_references;
    rhi_render_subpass_desc m_desc;

    render_pass* m_render_pass;
    std::size_t m_index;

    std::map<std::string, render_pipeline*> m_pipeline_map;
    std::vector<std::unique_ptr<render_pipeline>> m_pipelines;
};

class render_pass : public render_node
{
public:
    render_pass();
    virtual ~render_pass();

    render_attachment* add_attachment(std::string_view name);
    render_subpass* add_subpass(std::string_view name);

    render_pipeline* get_pipeline(std::string_view name) const;

    void add_dependency(
        render_subpass* src,
        rhi_pipeline_stage_flags src_stage,
        rhi_access_flags src_access,
        render_subpass* dst,
        rhi_pipeline_stage_flags dst_stage,
        rhi_access_flags dst_access);

    void add_camera(
        rhi_scissor_rect scissor,
        rhi_viewport viewport,
        rhi_parameter* parameter,
        rhi_framebuffer* framebuffer);

    virtual bool compile(compile_context& context) override;
    virtual void execute(execute_context& context) override;

    rhi_render_pass* get_interface() const noexcept { return m_interface.get(); }
    std::size_t get_attachment_count() const noexcept { return m_attachments.size(); }

private:
    struct render_camera
    {
        rhi_scissor_rect scissor;
        rhi_viewport viewport;
        rhi_parameter* parameter;
        rhi_framebuffer* framebuffer;
    };

    std::vector<std::unique_ptr<render_attachment>> m_attachments;
    std::vector<std::unique_ptr<render_subpass>> m_subpasses;
    std::vector<rhi_render_subpass_dependency_desc> m_dependencies;

    std::vector<render_camera> m_cameras;

    rhi_ptr<rhi_render_pass> m_interface;
};
} // namespace violet
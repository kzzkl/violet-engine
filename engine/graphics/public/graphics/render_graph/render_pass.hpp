#pragma once

#include "graphics/render_graph/pass.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <cassert>
#include <map>
#include <memory>
#include <vector>

namespace violet
{
struct render_subpass_reference
{
    pass_slot* slot;
    rhi_attachment_reference_type type;
    rhi_image_layout layout;
};

class render_pass : public pass
{
public:
    render_pass(renderer* renderer, setup_context& context);
    virtual ~render_pass();

    std::size_t add_subpass(const std::vector<render_subpass_reference>& references);

    render_pipeline* add_pipeline(std::size_t subpass_index);

    void add_dependency(
        std::size_t src_subpass,
        rhi_pipeline_stage_flags src_stage,
        rhi_access_flags src_access,
        std::size_t dst_subpass,
        rhi_pipeline_stage_flags dst_stage,
        rhi_access_flags dst_access);

    virtual bool compile(compile_context& context) override;
    virtual void execute(execute_context& context) override;

    rhi_render_pass* get_interface() const noexcept { return m_interface.get(); }

protected:
    rhi_framebuffer* get_framebuffer();

    rhi_resource_extent get_extent() const;

private:
    std::vector<rhi_render_subpass_dependency_desc> m_dependencies;

    std::vector<pass_slot*> m_framebuffer_slots;

    std::vector<rhi_render_subpass_desc> m_subpasses;
    std::vector<std::unique_ptr<render_pipeline>> m_pipelines;

    rhi_ptr<rhi_framebuffer> m_temporary_framebuffer;
    std::unordered_map<std::size_t, rhi_ptr<rhi_framebuffer>> m_framebuffer_cache;

    rhi_ptr<rhi_render_pass> m_interface;
    renderer* m_renderer;
};
} // namespace violet
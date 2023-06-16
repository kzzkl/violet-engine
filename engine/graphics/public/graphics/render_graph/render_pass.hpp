#pragma once

#include "graphics/render_graph/render_node.hpp"
#include "graphics/render_graph/render_pipeline.hpp"
#include <memory>
#include <vector>

namespace violet
{
class render_pass : public render_node
{
public:
    render_pass(std::string_view name, rhi_context* rhi);
    virtual ~render_pass();

    void set_attachment();

    void set_attachment_description(std::size_t index, const rhi_attachment_desc& desc);
    void set_attachment_count(std::size_t count);

    void set_subpass_references(
        std::size_t index,
        const std::vector<rhi_attachment_reference>& references);
    void set_subpass_count(std::size_t count);

    render_pipeline* add_pipeline(std::string_view name, std::size_t subpass);
    void remove_pipeline(render_pipeline* pipeline);

    virtual bool compile() override;

    rhi_render_pass* get_interface() const noexcept { return m_interface; }

private:
    struct subpass
    {
        std::vector<rhi_attachment_reference> references;
        std::vector<std::unique_ptr<render_pipeline>> pipelines;
    };

    std::vector<subpass> m_subpasses;
    std::vector<rhi_attachment_desc> m_attachment_desc;

    rhi_render_pass* m_interface;
};
} // namespace violet
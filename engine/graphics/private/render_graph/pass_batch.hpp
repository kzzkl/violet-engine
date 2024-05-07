#pragma once

#include "graphics/render_graph/pass.hpp"

namespace violet
{
class pass_batch
{
public:
    virtual ~pass_batch() {}

    virtual void execute(rhi_render_command* command, render_context* context) = 0;

private:
};

class render_pass_batch : public pass_batch
{
public:
    render_pass_batch(const std::vector<pass*> passes, renderer* renderer);

    virtual void execute(rhi_render_command* command, render_context* context) override;

private:
    static constexpr std::size_t FRAMEBUFFER_CLEANUP_INTERVAL = 50;

    struct framebuffer
    {
        rhi_ptr<rhi_framebuffer> framebuffer;
        bool is_used;
    };

    void cleanup_framebuffer();

    renderer* m_renderer;

    rhi_ptr<rhi_render_pass> m_render_pass;
    std::vector<std::vector<render_pass*>> m_passes;

    std::vector<resource*> m_attachments;

    std::unordered_map<std::size_t, framebuffer> m_framebuffer_cache;

    std::size_t m_execute_count;
};
} // namespace violet
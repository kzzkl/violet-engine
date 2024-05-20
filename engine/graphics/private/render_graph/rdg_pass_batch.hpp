#pragma once

#include "graphics/render_graph/rdg_pass.hpp"

namespace violet
{
class rdg_pass_batch
{
public:
    virtual ~rdg_pass_batch() {}

    virtual void execute(rhi_command* command, rdg_context* context) = 0;

private:
};

class rdg_render_pass_batch : public rdg_pass_batch
{
public:
    rdg_render_pass_batch(const std::vector<rdg_pass*> passes, render_device* device);

    virtual void execute(rhi_command* command, rdg_context* context) override;

private:
    static constexpr std::size_t FRAMEBUFFER_CLEANUP_INTERVAL = 50;

    struct framebuffer
    {
        rhi_ptr<rhi_framebuffer> framebuffer;
        bool is_used;
    };

    void cleanup_framebuffer();

    render_device* m_device;

    rhi_ptr<rhi_render_pass> m_render_pass;
    std::vector<std::vector<rdg_render_pass*>> m_passes;

    std::vector<rdg_resource*> m_attachments;

    std::unordered_map<std::size_t, framebuffer> m_framebuffer_cache;

    std::size_t m_execute_count;
};

class rdg_compute_pass_batch : public rdg_pass_batch
{
public:
    rdg_compute_pass_batch(const std::vector<rdg_pass*> passes, render_device* device);

    virtual void execute(rhi_command* command, rdg_context* context) override;

private:
    const std::vector<rdg_pass*> m_passes;
};

class rdg_other_pass_batch : public rdg_pass_batch
{
public:
    rdg_other_pass_batch(const std::vector<rdg_pass*> passes, render_device* device);

    virtual void execute(rhi_command* command, rdg_context* context) override;

private:
    const std::vector<rdg_pass*> m_passes;
};
} // namespace violet
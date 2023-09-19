#pragma once

#include "graphics/render_graph/material.hpp"
#include "graphics/render_graph/render_pass.hpp"
#include <map>
#include <memory>

namespace violet
{
class render_graph
{
public:
    render_graph(rhi_context* rhi);
    render_graph(const render_graph&) = delete;
    ~render_graph();

    render_pass* add_render_pass(std::string_view name);

    material_layout* add_material_layout(std::string_view name);

    bool compile();
    void execute();

    rhi_semaphore* get_render_finished_semaphore() const;

    render_graph& operator=(const render_graph&) = delete;

private:
    std::vector<std::unique_ptr<render_pass>> m_render_passes;
    std::vector<rhi_semaphore*> m_render_finished_semaphores;

    std::map<std::string, std::unique_ptr<material_layout>> m_material_layouts;

    rhi_context* m_rhi;
};
} // namespace violet
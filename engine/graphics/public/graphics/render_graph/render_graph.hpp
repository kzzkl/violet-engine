#pragma once

#include "graphics/graphics_context.hpp"
#include "graphics/render_graph/material.hpp"
#include "graphics/render_graph/render_pass.hpp"
#include <memory>

namespace violet
{
class render_graph
{
public:
    render_graph(graphics_context* context);
    render_graph(const render_graph&) = delete;
    ~render_graph();

    render_pass* add_render_pass(std::string_view name);

    material_layout* add_material_layout(std::string_view name);
    material_layout* get_material_layout(std::string_view name) const;

    bool compile();
    void execute();

    rhi_semaphore* get_render_finished_semaphore() const;

    render_graph& operator=(const render_graph&) = delete;

private:
    std::vector<std::unique_ptr<render_pass>> m_render_passes;
    std::vector<rhi_semaphore*> m_render_finished_semaphores;

    std::map<std::string, std::unique_ptr<material_layout>> m_material_layouts;

    graphics_context* m_context;
};
} // namespace violet
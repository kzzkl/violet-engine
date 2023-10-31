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
    virtual ~render_graph();

    render_pass* add_render_pass(std::string_view name);
    render_pass* get_render_pass(std::string_view name) const;

    render_pipeline* get_pipeline(std::string_view name) const;

    material_layout* add_material_layout(std::string_view name);
    material_layout* get_material_layout(std::string_view name) const;

    material* add_material(material_layout* layout, std::string_view name);
    material* add_material(std::string_view layout, std::string_view name);
    material* get_material(std::string_view layout, std::string_view name) const;

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
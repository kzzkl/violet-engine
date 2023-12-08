#pragma once

#include "graphics/render_graph/compute_pass.hpp"
#include "graphics/render_graph/material.hpp"
#include "graphics/render_graph/render_pass.hpp"
#include "graphics/renderer.hpp"
#include <memory>

namespace violet
{
class render_graph
{
public:
    render_graph(renderer* renderer);
    render_graph(const render_graph&) = delete;
    virtual ~render_graph();

    render_pass* add_render_pass(std::string_view name);
    render_pass* get_render_pass(std::string_view name) const;
    render_pipeline* get_render_pipeline(std::string_view name) const;

    material_layout* add_material_layout(std::string_view name);
    material_layout* get_material_layout(std::string_view name) const;

    material* add_material(std::string_view name, material_layout* layout);
    material* add_material(std::string_view name, std::string_view layout);
    material* get_material(std::string_view name, std::string_view layout) const;

    compute_pass* add_compute_pass(std::string_view name);
    compute_pass* get_compute_pass(std::string_view name) const;
    compute_pipeline* get_compute_pipeline(std::string_view name) const;

    bool compile();
    void execute(rhi_parameter* light);

    rhi_semaphore* get_render_finished_semaphore() const;

    render_graph& operator=(const render_graph&) = delete;

private:
    std::vector<rhi_ptr<rhi_semaphore>> m_render_finished_semaphores;

    std::vector<std::unique_ptr<render_pass>> m_render_passes;
    std::vector<std::unique_ptr<render_pipeline>> m_render_pipelines;
    std::vector<std::unique_ptr<material_layout>> m_material_layouts;

    std::vector<std::unique_ptr<compute_pass>> m_compute_passes;

    renderer* m_renderer;
};
} // namespace violet
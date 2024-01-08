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

    pass_slot* add_slot(std::string_view name, pass_slot_type type);
    pass_slot* get_slot(std::string_view name) const;

    template <typename T, typename... Args>
    T* add_pass(std::string_view name, Args&&... args)
    {
        assert(m_passes.find(name.data()) == m_passes.end());

        setup_context setup_context(m_material_pipelines);
        auto pass = std::make_unique<T>(m_renderer, setup_context, std::forward<Args>(args)...);

        T* result = pass.get();
        m_passes[name.data()] = std::move(pass);
        return result;
    }

    template <typename T, typename... Args>
    T* add_material(std::string_view name, Args&&... args)
    {
        assert(m_materials.find(name.data()) == m_materials.end());

        auto material = std::make_unique<T>(std::forward<Args>(args)...);
        auto layout = material->get_layout();

        for (auto& pipeline_name : layout)
        {
            auto [pipeline, parameter_layout] = m_material_pipelines.at(pipeline_name);
            material->m_pipelines.push_back(pipeline);
            material->m_parameters.push_back(m_renderer->create_parameter(parameter_layout));
        }

        T* result = material.get();
        m_materials[name.data()] = std::move(material);
        return result;
    }

    template <typename T>
    material* get_material(std::string_view name) const
    {
        return static_cast<T*>(m_materials.at(name.data()));
    }

    void set_light(rhi_parameter* light) noexcept { m_light = light; }
    void set_camera(std::string_view name, rhi_parameter* parameter)
    {
        m_cameras[name.data()] = parameter;
    }

    bool compile();
    void execute();

    rhi_semaphore* get_render_finished_semaphore() const;

    render_graph& operator=(const render_graph&) = delete;

private:
    std::map<std::string, std::unique_ptr<pass_slot>> m_slots;
    std::map<std::string, std::unique_ptr<pass>> m_passes;

    std::vector<pass*> m_execute_list;

    std::map<std::string, std::unique_ptr<material>> m_materials;
    std::map<std::string, std::pair<render_pipeline*, rhi_parameter_layout*>> m_material_pipelines;

    std::unordered_map<std::string, rhi_parameter*> m_cameras;
    rhi_parameter* m_light;

    pass_slot* m_back_buffer;

    std::vector<rhi_ptr<rhi_semaphore>> m_render_finished_semaphores;
    renderer* m_renderer;
};
} // namespace violet
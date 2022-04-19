#pragma once

#include "camera.hpp"
#include "context.hpp"
#include "debug_pipeline.hpp"
#include "graphics_config.hpp"
#include "graphics_plugin.hpp"
#include "render_parameter.hpp"
#include "render_pipeline.hpp"
#include "transform.hpp"
#include "type_trait.hpp"
#include "view.hpp"
#include "visual.hpp"
#include <set>

namespace ash::graphics
{
class graphics : public ash::core::system_base
{
public:
    static constexpr const char* TASK_RENDER = "graphics render";

    template <typename T>
    using interface_map = std::unordered_map<std::string, std::unique_ptr<T>>;

public:
    graphics() noexcept;

    virtual bool initialize(const dictionary& config) override;

    template <typename T, typename... Args>
    std::unique_ptr<T> make_render_pipeline(std::string_view name, Args&&... args)
    {
        pipeline_layout* layout;
        pipeline* pipeline;

        auto [make_result, unit_conut, pass_count] = make_pipeline(name, layout, pipeline);

        if (!make_result)
            return false;

        auto result = std::make_unique<T>(layout, pipeline, std::forward<Args>(args)...);
        result->parameter_count(unit_conut, pass_count);
        return result;
    }

    std::unique_ptr<render_parameter> make_render_parameter(std::string_view name)
    {
        auto [fount, desc, _] = m_config.find_desc<pipeline_parameter_desc>(name);
        if (!fount)
        {
            log::error("render parameter no found: {}", name);
            return nullptr;
        }

        return std::make_unique<render_parameter>(m_factory->make_pipeline_parameter(desc));
    }

    template <typename Vertex>
    std::unique_ptr<resource> make_vertex_buffer(
        const Vertex* data,
        std::size_t size,
        bool dynamic = false)
    {
        vertex_buffer_desc desc;
        if (dynamic)
            desc = {nullptr, sizeof(Vertex), size, dynamic};
        else
            desc = {data, sizeof(Vertex), size, dynamic};
        return std::unique_ptr<resource>(m_factory->make_vertex_buffer(desc));
    }

    template <typename Index>
    std::unique_ptr<resource> make_index_buffer(const Index* data, std::size_t size)
    {
        index_buffer_desc desc = {data, sizeof(Index), size};
        return std::unique_ptr<resource>(m_factory->make_index_buffer(desc));
    }

    std::unique_ptr<resource> make_texture(std::string_view file);

    debug_pipeline& debug() { return *m_debug; }

private:
    std::tuple<bool, std::size_t, std::size_t> make_pipeline(
        std::string_view name,
        pipeline_layout*& layout,
        pipeline*& pipeline);

    bool initialize_resource();
    void initialize_debug();

    void update();
    void render();

    void render_debug();

    graphics_plugin m_plugin;
    renderer* m_renderer;
    factory* m_factory;

    ash::ecs::view<visual, scene::transform>* m_object_view;
    ash::ecs::view<main_camera, camera, scene::transform>* m_camera_view;

    //ash::ecs::view<scene::transform>* m_tv;

    std::unique_ptr<render_parameter> m_parameter_pass;
    std::set<render_pipeline*> m_render_pipelines;

    graphics_config m_config;
    std::unique_ptr<debug_pipeline> m_debug;
};
} // namespace ash::graphics
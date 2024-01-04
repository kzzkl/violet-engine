#pragma once

#include "graphics/render_graph/render_pipeline.hpp"
#include "graphics/render_graph/render_resource.hpp"
#include "graphics/renderer.hpp"
#include <unordered_map>

namespace violet
{
class setup_context
{
public:
    struct material_pipeline
    {
        std::string name;
        render_pipeline* pipeline;
        rhi_parameter_layout* layout;
    };

public:
    setup_context(
        const std::map<std::string, std::unique_ptr<render_resource>>& resources,
        std::map<std::string, std::pair<render_pipeline*, rhi_parameter_layout*>>&
            material_pipelines);

    render_resource* read(std::string_view name);
    render_resource* write(std::string_view name);

    void register_material_pipeline(
        std::string_view name,
        render_pipeline* pipeline,
        rhi_parameter_layout* layout);

private:
    std::map<std::string, std::pair<render_pipeline*, rhi_parameter_layout*>>& m_material_pipelines;
    const std::map<std::string, std::unique_ptr<render_resource>>& m_resources;
};

class compile_context
{
public:
    renderer* renderer;
};

class execute_context
{
public:
    execute_context(
        rhi_render_command* command,
        rhi_parameter* light,
        const std::unordered_map<std::string, rhi_parameter*>& cameras);

    rhi_render_command* get_command() const noexcept { return m_command; }
    rhi_parameter* get_light() const noexcept { return m_light; }
    rhi_parameter* get_camera(std::string_view name) const { return m_cameras.at(name.data()); }

private:
    rhi_render_command* m_command;
    rhi_parameter* m_light;
    const std::unordered_map<std::string, rhi_parameter*>& m_cameras;
};

class pass
{
public:
    pass(renderer* renderer, setup_context& context);
    pass(const pass&) = delete;
    virtual ~pass();

    pass& operator=(const renderer&) = delete;

    virtual bool compile(compile_context& context) = 0;
    virtual void execute(execute_context& context) = 0;
};
} // namespace violet
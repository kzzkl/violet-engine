#pragma once

#include "graphics/render_graph/render_pipeline.hpp"
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
    setup_context(std::map<std::string, std::pair<render_pipeline*, rhi_parameter_layout*>>&
                      material_pipelines);

    void register_material_pipeline(
        std::string_view name,
        render_pipeline* pipeline,
        rhi_parameter_layout* layout);

private:
    std::map<std::string, std::pair<render_pipeline*, rhi_parameter_layout*>>& m_material_pipelines;
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

class pass_slot
{
public:
    pass_slot(std::string_view name, std::size_t index, bool clear = false) noexcept;

    void set_format(rhi_resource_format format) noexcept { m_format = format; }
    rhi_resource_format get_format() const noexcept { return m_format; }

    void set_samples(rhi_sample_count samples) noexcept { m_samples = samples; }
    rhi_sample_count get_samples() const noexcept { return m_samples; }

    void set_input_layout(rhi_image_layout layout) noexcept { m_input_layout = layout; }
    rhi_image_layout get_input_layout() const noexcept { return m_input_layout; }
    rhi_image_layout get_output_layout() const noexcept
    {
        return m_connections.empty() ? m_input_layout : m_connections[0]->get_input_layout();
    }

    bool is_clear() const noexcept { return m_clear; }

    const std::string& get_name() const noexcept { return m_name; }
    std::size_t get_index() const noexcept { return m_index; }

    bool connect(pass_slot* slot);

    void set_image(rhi_image* image, bool framebuffer_cache = true);
    rhi_image* get_image() const noexcept { return m_image; }

    bool is_framebuffer_cache() const noexcept { return m_framebuffer_cache; }

private:
    std::string m_name;
    std::size_t m_index;

    rhi_resource_format m_format;
    rhi_sample_count m_samples;

    rhi_image_layout m_input_layout;

    bool m_clear;

    std::vector<pass_slot*> m_connections;

    rhi_image* m_image;
    bool m_framebuffer_cache;
};

class pass
{
public:
    pass(renderer* renderer, setup_context& context);

    pass_slot* add_slot(std::string_view name, bool clear = false);
    pass_slot* get_slot(std::string_view name) const;
    const std::vector<std::unique_ptr<pass_slot>>& get_slots() const noexcept { return m_slots; }

    virtual bool compile(compile_context& context) { return true; }
    virtual void execute(execute_context& context) = 0;

private:
    std::vector<std::unique_ptr<pass_slot>> m_slots;
};
} // namespace violet
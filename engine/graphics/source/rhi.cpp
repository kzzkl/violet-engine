#include "graphics/rhi.hpp"
#include "assert.hpp"
#include "core/plugin.hpp"
#include "log.hpp"

namespace ash::graphics
{
render_pass_info::render_pass_info()
{
    blend.enable = false;
    depth_stencil.depth_functor = depth_functor::LESS;
    rasterizer.cull_mode = cull_mode::BACK;
}

class rhi_plugin : public core::plugin
{
public:
    rhi_plugin() {}

    rhi_interface& rhi_impl() { return *m_rhi_impl; }

protected:
    virtual bool on_load() override
    {
        make_rhi make = static_cast<make_rhi>(find_symbol("make_rhi"));
        if (make == nullptr)
        {
            log::error("Symbol not found in plugin: make_rhi.");
            return false;
        }

        m_rhi_impl.reset(make());

        return true;
    }

    virtual void on_unload() override { m_rhi_impl = nullptr; }

private:
    std::unique_ptr<rhi_interface> m_rhi_impl;
};

rhi& rhi::instance()
{
    static rhi instance;
    return instance;
}

void rhi::initialize(std::string_view plugin, const rhi_info& info)
{
    instance().m_plugin = std::make_unique<rhi_plugin>();
    instance().m_plugin->load(plugin);

    impl().initialize(info);

    instance().m_renderer.reset(impl().make_renderer());
}

renderer_interface& rhi::renderer()
{
    return *instance().m_renderer;
}

resource_format rhi::back_buffer_format()
{
    return instance().m_renderer->back_buffer()->format();
}

void rhi::register_pipeline_parameter_layout(
    std::string_view name,
    const std::vector<pipeline_parameter_pair>& parameters)
{
    pipeline_parameter_layout_desc desc = {};
    for (auto& parameter : parameters)
    {
        desc.parameters[desc.parameter_count] = parameter;
        ++desc.parameter_count;
    }
    auto interface = impl().make_pipeline_parameter_layout(desc);
    instance().m_parameter_layouts[name.data()].reset(interface);
}

std::unique_ptr<render_pipeline_interface> rhi::make_render_pipeline(
    const render_pipeline_info& info)
{
    render_pipeline_desc desc = {};
    for (auto& attachment : info.attachments)
    {
        desc.attachments[desc.attachment_count] = attachment;
        ++desc.attachment_count;
    }

    for (auto& pass : info.passes)
    {
        render_pass_desc& pass_desc = desc.passes[desc.pass_count];
        ++desc.pass_count;

        for (auto& vertex_attribute : pass.vertex_attributes)
        {
            pass_desc.vertex_attributes[pass_desc.vertex_attribute_count] = vertex_attribute;
            ++pass_desc.vertex_attribute_count;
        }

        for (auto& parameter : pass.parameters)
        {
            pass_desc.parameters[pass_desc.parameter_count] =
                find_pipeline_parameter_layout(parameter);
            ++pass_desc.parameter_count;
        }

        for (auto& reference : pass.references)
        {
            pass_desc.references[pass_desc.reference_count] = reference;
            ++pass_desc.reference_count;
        }

        pass_desc.vertex_shader = pass.vertex_shader.c_str();
        pass_desc.pixel_shader = pass.pixel_shader.c_str();

        pass_desc.blend = pass.blend;
        pass_desc.depth_stencil = pass.depth_stencil;
        pass_desc.rasterizer = pass.rasterizer;
        pass_desc.primitive_topology = pass.primitive_topology;
        pass_desc.samples = pass.samples;
    }

    return std::unique_ptr<render_pipeline_interface>(impl().make_render_pipeline(desc));
}

std::unique_ptr<compute_pipeline_interface> rhi::make_compute_pipeline(
    const compute_pipeline_info& info)
{
    compute_pipeline_desc desc = {};
    desc.compute_shader = info.compute_shader.c_str();

    for (auto& parameter : info.parameters)
    {
        desc.parameters[desc.parameter_count] = find_pipeline_parameter_layout(parameter);
        ++desc.parameter_count;
    }

    return std::unique_ptr<compute_pipeline_interface>(impl().make_compute_pipeline(desc));
}

std::unique_ptr<pipeline_parameter_interface> rhi::make_pipeline_parameter(std::string_view name)
{
    auto layout = find_pipeline_parameter_layout(name);
    ASH_ASSERT(layout);
    return std::unique_ptr<pipeline_parameter_interface>(impl().make_pipeline_parameter(layout));
}

std::unique_ptr<resource_interface> rhi::make_texture(
    const std::uint8_t* data,
    std::uint32_t width,
    std::uint32_t height,
    resource_format format)
{
    return std::unique_ptr<resource_interface>(impl().make_texture(data, width, height, format));
}

std::unique_ptr<resource_interface> rhi::make_render_target(const render_target_desc& desc)
{
    return std::unique_ptr<resource_interface>(impl().make_render_target(desc));
}

std::unique_ptr<resource_interface> rhi::make_depth_stencil_buffer(
    const depth_stencil_buffer_desc& desc)
{
    return std::unique_ptr<resource_interface>(impl().make_depth_stencil_buffer(desc));
}

std::unique_ptr<resource_interface> rhi::make_texture(std::string_view file)
{
    return std::unique_ptr<resource_interface>(impl().make_texture(file.data()));
}

rhi_interface& rhi::impl()
{
    return instance().m_plugin->rhi_impl();
}

pipeline_parameter_layout_interface* rhi::find_pipeline_parameter_layout(std::string_view name)
{
    auto iter = instance().m_parameter_layouts.find(name.data());
    if (iter != instance().m_parameter_layouts.end())
        return iter->second.get();
    else
        return nullptr;
}
} // namespace ash::graphics
#include "d3d12_context.hpp"
#include "d3d12_pipeline.hpp"
#include "graphics_interface.hpp"
#include <cstring>

using namespace ash::graphics::d3d12;

namespace ash::graphics::d3d12
{
class d3d12_factory : public factory
{
public:
    virtual pipeline_parameter* make_pipeline_parameter(
        const pipeline_parameter_desc& desc) override
    {
        return new d3d12_pipeline_parameter(desc);
    }

    virtual pipeline_layout* make_pipeline_layout(const pipeline_layout_desc& desc) override
    {
        return new d3d12_parameter_layout(desc);
    }

    virtual pipeline* make_pipeline(const pipeline_desc& desc) override
    {
        return new d3d12_pipeline(desc);
    }

    virtual resource* make_upload_buffer(std::size_t size) override
    {
        return new d3d12_upload_buffer(size);
    }

    virtual resource* make_vertex_buffer(const vertex_buffer_desc& desc) override
    {
        auto command_list = d3d12_context::command()->allocate_dynamic_command();
        d3d12_vertex_buffer* result = new d3d12_vertex_buffer(
            desc.vertices,
            desc.vertex_size,
            desc.vertex_count,
            command_list.get());
        d3d12_context::command()->execute_command(command_list);
        return result;
    }

    virtual resource* make_index_buffer(const index_buffer_desc& desc) override
    {
        auto command_list = d3d12_context::command()->allocate_dynamic_command();
        d3d12_index_buffer* result = new d3d12_index_buffer(
            desc.indices,
            desc.index_size,
            desc.index_count,
            command_list.get());
        d3d12_context::command()->execute_command(command_list);
        return result;
    }

    virtual resource* make_texture(const std::uint8_t* data, std::size_t size) override
    {
        auto command_list = d3d12_context::command()->allocate_dynamic_command();
        d3d12_texture* result = new d3d12_texture(data, size, command_list.get());
        d3d12_context::command()->execute_command(command_list);
        return result;
    }
};

class d3d12_context_wrapper : public context
{
public:
    virtual bool initialize(const context_config& config) override
    {
        m_factory = std::make_unique<d3d12_factory>();
        return d3d12_context::initialize(config);
    }

    virtual renderer_type* renderer() override { return d3d12_context::renderer(); }
    virtual factory_type* factory() { return m_factory.get(); }

private:
    std::unique_ptr<d3d12_factory> m_factory;
};
} // namespace ash::graphics::d3d12

extern "C"
{
    PLUGIN_API ash::core::plugin_info get_plugin_info()
    {
        ash::core::plugin_info info = {};

        char name[] = "graphics-d3d12";
        memcpy(info.name, name, sizeof(name));

        info.version.major = 1;
        info.version.minor = 0;

        return info;
    }

    PLUGIN_API ash::graphics::context* make_context() { return new d3d12_context_wrapper(); }
}
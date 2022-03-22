#pragma once

#include "plugin_interface.hpp"
#include <cstddef>
#include <cstdint>

namespace ash::graphics
{
template <typename T, std::size_t Size>
struct list
{
    T data[Size];
    std::size_t size;
};

enum class primitive_topology_type : std::uint8_t
{
    TRIANGLE_LIST,
    LINE_LIST
};

class resource
{
public:
    virtual ~resource() = default;

    virtual void upload(const void* data, std::size_t size, std::size_t offset = 0) {}
};

enum class pipeline_parameter_type : std::uint8_t
{
    TEXTURE,
    BUFFER
};

using pipeline_parameter_desc = list<pipeline_parameter_type, 16>;

class pipeline_parameter
{
public:
    virtual ~pipeline_parameter() = default;

    virtual void bind(std::size_t index, resource* data, std::size_t offset = 0) = 0;
};

using pipeline_layout_desc = list<pipeline_parameter_desc, 16>;

class pipeline_layout
{
public:
    virtual ~pipeline_layout() = default;
};

enum class vertex_attribute_type : std::uint8_t
{
    INT,
    INT2,
    INT3,
    INT4,
    UINT,
    UINT2,
    UINT3,
    UINT4,
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4
};

struct vertex_attribute_desc
{
    char name[32];
    vertex_attribute_type type;
    std::uint32_t index;
};

struct pipeline_desc
{
    char name[32];

    list<vertex_attribute_desc, 16> vertex_layout;
    pipeline_layout* layout;

    char vertex_shader[128];
    char pixel_shader[128];
};

class pipeline
{
public:
};

class render_command
{
public:
    using pipeline_type = pipeline;
    using layout_type = pipeline_layout;

public:
    virtual ~render_command() = default;

    virtual void pipeline(pipeline_type* pipeline) = 0;
    virtual void layout(layout_type* layout) = 0;
    virtual void parameter(std::size_t index, pipeline_parameter* parameter) = 0;

    virtual void draw(
        resource* vertex,
        resource* index,
        primitive_topology_type primitive_topology,
        resource* target) = 0;
};

struct adapter_info
{
    char description[128];
};

class renderer
{
public:
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual render_command* allocate_command() = 0;
    virtual void execute(render_command* command) = 0;

    virtual resource* back_buffer() = 0;
    virtual std::size_t adapter(adapter_info* infos, std::size_t size) const = 0;

private:
};

struct vertex_buffer_desc
{
    const void* vertices;
    std::size_t vertex_size;
    std::size_t vertex_count;
};

struct index_buffer_desc
{
    const void* indices;
    std::size_t index_size;
    std::size_t index_count;
};

class factory
{
public:
    virtual pipeline_parameter* make_pipeline_parameter(const pipeline_parameter_desc& desc) = 0;
    virtual pipeline_layout* make_pipeline_layout(const pipeline_layout_desc& desc) = 0;
    virtual pipeline* make_pipeline(const pipeline_desc& desc) = 0;

    virtual resource* make_upload_buffer(std::size_t size) = 0;

    virtual resource* make_vertex_buffer(const vertex_buffer_desc& desc) = 0;
    virtual resource* make_index_buffer(const index_buffer_desc& desc) = 0;

private:
};

struct context_config
{
    std::uint32_t width;
    std::uint32_t height;

    const void* window_handle;

    bool msaa_4x;
    std::size_t render_concurrency;
};

class context
{
public:
    using renderer_type = renderer;
    using factory_type = factory;

public:
    virtual ~context() = default;

    virtual bool initialize(const context_config& config) = 0;

    virtual renderer_type* renderer() = 0;
    virtual factory_type* factory() = 0;
};

using make_context = context* (*)();
} // namespace ash::graphics
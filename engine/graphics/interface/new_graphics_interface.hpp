#pragma once

#include "math.hpp"
#include "plugin_interface.hpp"
#include <cstddef>
#include <cstdint>

namespace ash::graphics::vk
{
class resource_interface
{
};

enum class vertex_attribute_type : std::uint8_t
{
    INT,    // R32 SINT
    INT2,   // R32G32 SINT
    INT3,   // R32G32B32 SINT
    INT4,   // R32G32B32A32 SINT
    UINT,   // R32 UINT
    UINT2,  // R32G32 UINT
    UINT3,  // R32G32B32 UINT
    UINT4,  // R32G32B32A32 UINT
    FLOAT,  // R32 FLOAT
    FLOAT2, // R32G32 FLOAT
    FLOAT3, // R32G32B32 FLOAT
    FLOAT4, // R32G32B32A32 FLOAT
    COLOR   // R8G8B8A8
};

struct vertex_attribute_desc
{
    vertex_attribute_type type;
    std::size_t offset;
};

struct vertex_layout_desc
{
    vertex_attribute_desc* attributes;
    std::size_t attribute_count;
};

enum class pipeline_parameter_type : std::uint8_t
{
    BOOL,
    UINT,
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    FLOAT4x4,
    FLOAT4x4_ARRAY,
    TEXTURE
};

struct pipeline_parameter_pair
{
    pipeline_parameter_type type;
    std::size_t size; // Only for array.
};

struct pipeline_parameter_layout_desc
{
    pipeline_parameter_pair* parameter;
    std::size_t size;
};

class pipeline_parameter_layout_interface
{
};

struct pipeline_layout_desc
{
    pipeline_parameter_layout_interface* parameter;
    std::size_t size;
};

class pipeline_layout_interface
{
};

class pipeline_parameter_interface
{
public:
    virtual ~pipeline_parameter_interface() = default;

    virtual void set(std::size_t index, bool value) = 0;
    virtual void set(std::size_t index, std::uint32_t value) = 0;
    virtual void set(std::size_t index, float value) = 0;
    virtual void set(std::size_t index, const math::float2& value) = 0;
    virtual void set(std::size_t index, const math::float3& value) = 0;
    virtual void set(std::size_t index, const math::float4& value) = 0;
    virtual void set(std::size_t index, const math::float4x4& value) = 0;
    virtual void set(std::size_t index, const math::float4x4* data, size_t size) = 0;
    virtual void set(std::size_t index, resource_interface* texture) = 0;
};

struct pipeline_blend_desc
{
    enum class factor_type
    {
        ZERO,
        ONE,
        SOURCE_COLOR,
        SOURCE_ALPHA,
        SOURCE_INV_ALPHA,
        TARGET_COLOR,
        TARGET_ALPHA,
        TARGET_INV_ALPHA
    };

    enum class op_type
    {
        ADD,
        SUBTRACT,
        MIN,
        MAX
    };

    bool enable;

    factor_type source_factor;
    factor_type target_factor;
    op_type op;

    factor_type source_alpha_factor;
    factor_type target_alpha_factor;
    op_type alpha_op;
};

struct pipeline_depth_stencil_desc
{
    enum class depth_functor_type
    {
        LESS,
        ALWAYS
    };

    depth_functor_type depth_functor;
};

enum class primitive_topology_type : std::uint8_t
{
    TRIANGLE_LIST,
    LINE_LIST
};

struct pipeline_desc
{
    const char* vertex_shader;
    const char* pixel_shader;

    vertex_layout_desc vertex_layout;
    pipeline_layout_interface* pipeline_layout;

    pipeline_blend_desc blend;
    pipeline_depth_stencil_desc depth_stencil;

    std::size_t* input;
    std::size_t input_count;

    std::size_t* output;
    std::size_t output_count;

    std::size_t depth;
    bool output_depth;

    primitive_topology_type primitive_topology;
};

struct render_pass_desc
{
    pipeline_desc* subpasses;
    std::size_t subpass_count;
};

class render_pass_interface
{
};

struct frame_buffer_desc
{
    resource_interface** resources;
    std::size_t resource_count;

    std::uint32_t width;
    std::uint32_t height;

    render_pass_interface* render_pass;
};

class frame_buffer_interface
{
};

class render_command_interface
{
public:
    virtual void begin(render_pass_interface* pass, frame_buffer_interface* frame_buffer) = 0;
    virtual void end(render_pass_interface* pass) = 0;
    virtual void next(render_pass_interface* pass) = 0;

    virtual void parameter(std::size_t i, pipeline_parameter_interface*) = 0;

    virtual void draw(
        resource_interface* vertex,
        resource_interface* index,
        std::size_t index_start,
        std::size_t index_end,
        std::size_t vertex_base) = 0;
};

struct renderer_desc
{
    std::uint32_t width;
    std::uint32_t height;

    void* window_handle;

    std::size_t multiple_sampling{1};
    std::size_t frame_resource{3};
    std::size_t render_concurrency{4};
};

class renderer_interface
{
public:
    virtual std::size_t begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual render_command_interface* allocate_command() = 0;
    virtual void execute(render_command_interface*) = 0;

    virtual resource_interface* back_buffer(std::size_t index) = 0;
    virtual std::size_t back_buffer_count() = 0;

    virtual resource_interface* depth_stencil() = 0;
};

struct vertex_buffer_desc
{
    const void* vertices;
    std::size_t vertex_size;
    std::size_t vertex_count;
    bool dynamic;
};

struct index_buffer_desc
{
    const void* indices;
    std::size_t index_size;
    std::size_t index_count;
    bool dynamic;
};

class factory_interface
{
public:
    virtual renderer_interface* make_renderer(const renderer_desc& desc) = 0;

    virtual frame_buffer_interface* make_frame_buffer(const frame_buffer_desc& desc) = 0;
    virtual render_pass_interface* make_render_pass(const render_pass_desc& desc) = 0;

    virtual pipeline_parameter_layout_interface* make_pipeline_parameter_layout(
        const pipeline_parameter_layout_desc& desc) = 0;
    virtual pipeline_layout_interface* make_pipeline_layout(const pipeline_layout_desc& desc) = 0;
    virtual pipeline_parameter_interface* make_pipeline_parameter(
        pipeline_parameter_layout_interface* layout) = 0;

    virtual resource_interface* make_vertex_buffer(const vertex_buffer_desc& desc) = 0;
    virtual resource_interface* make_index_buffer(const index_buffer_desc& desc) = 0;

    virtual resource_interface* make_texture(const char* file) = 0;

    virtual resource_interface* make_render_target(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling = 1) = 0;
    virtual resource_interface* make_depth_stencil(
        std::uint32_t width,
        std::uint32_t height,
        std::size_t multiple_sampling = 1) = 0;
};

using make_factory = factory_interface* (*)();
} // namespace ash::graphics::vk
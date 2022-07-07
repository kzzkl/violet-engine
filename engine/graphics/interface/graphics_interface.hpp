#pragma once

#include "math/math.hpp"
#include "plugin_interface.hpp"
#include <cstddef>
#include <cstdint>

namespace ash::graphics
{
enum resource_format
{
    RESOURCE_FORMAT_UNDEFINED,

    RESOURCE_FORMAT_R8_UNORM,
    RESOURCE_FORMAT_R8_UINT,

    RESOURCE_FORMAT_R8G8B8A8_UNORM,
    RESOURCE_FORMAT_B8G8R8A8_UNORM,

    RESOURCE_FORMAT_R32G32B32A32_FLOAT,
    RESOURCE_FORMAT_R32G32B32A32_SINT,
    RESOURCE_FORMAT_R32G32B32A32_UINT,

    RESOURCE_FORMAT_D24_UNORM_S8_UINT
};

enum resource_state
{
    RESOURCE_STATE_UNDEFINED,
    RESOURCE_STATE_RENDER_TARGET,
    RESOURCE_STATE_DEPTH_STENCIL,
    RESOURCE_STATE_PRESENT
};

struct resource_extent
{
    std::uint32_t width;
    std::uint32_t height;
};

class resource_interface
{
public:
    virtual ~resource_interface() = default;

    virtual resource_format format() const noexcept = 0;

    virtual resource_extent extent() const noexcept = 0;
    virtual std::size_t size() const noexcept = 0;

    virtual void upload(const void* data, std::size_t size, std::size_t offset = 0) {}
};

enum vertex_attribute_type
{
    VERTEX_ATTRIBUTE_TYPE_INT,    // R32 INT
    VERTEX_ATTRIBUTE_TYPE_INT2,   // R32G32 INT
    VERTEX_ATTRIBUTE_TYPE_INT3,   // R32G32B32 INT
    VERTEX_ATTRIBUTE_TYPE_INT4,   // R32G32B32A32 INT
    VERTEX_ATTRIBUTE_TYPE_UINT,   // R32 UINT
    VERTEX_ATTRIBUTE_TYPE_UINT2,  // R32G32 UINT
    VERTEX_ATTRIBUTE_TYPE_UINT3,  // R32G32B32 UINT
    VERTEX_ATTRIBUTE_TYPE_UINT4,  // R32G32B32A32 UINT
    VERTEX_ATTRIBUTE_TYPE_FLOAT,  // R32 FLOAT
    VERTEX_ATTRIBUTE_TYPE_FLOAT2, // R32G32 FLOAT
    VERTEX_ATTRIBUTE_TYPE_FLOAT3, // R32G32B32 FLOAT
    VERTEX_ATTRIBUTE_TYPE_FLOAT4, // R32G32B32A32 FLOAT
    VERTEX_ATTRIBUTE_TYPE_COLOR   // R8G8B8A8
};

struct vertex_attribute
{
    const char* name;
    vertex_attribute_type type;
};

enum pipeline_parameter_type
{
    PIPELINE_PARAMETER_TYPE_CONSTANT_BUFFER,
    PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE,
    PIPELINE_PARAMETER_TYPE_UNORDERED_ACCESS
};

struct pipeline_parameter_pair
{
    pipeline_parameter_type type;
    std::size_t size;
};

struct pipeline_parameter_layout_desc
{
    pipeline_parameter_pair parameters[16];
    std::size_t parameter_count;
};

class pipeline_parameter_layout_interface
{
public:
    virtual ~pipeline_parameter_layout_interface() = default;
};

class pipeline_parameter_interface
{
public:
    virtual ~pipeline_parameter_interface() = default;

    virtual void set(std::size_t index, const void* data, size_t size) = 0;
    virtual void set(std::size_t index, resource_interface* texture) = 0;

    virtual void* constant_buffer_pointer(std::size_t index) = 0;
};

enum blend_factor
{
    BLEND_FACTOR_ZERO,
    BLEND_FACTOR_ONE,
    BLEND_FACTOR_SOURCE_COLOR,
    BLEND_FACTOR_SOURCE_ALPHA,
    BLEND_FACTOR_SOURCE_INV_ALPHA,
    BLEND_FACTOR_TARGET_COLOR,
    BLEND_FACTOR_TARGET_ALPHA,
    BLEND_FACTOR_TARGET_INV_ALPHA
};

enum blend_op
{
    BLEND_OP_ADD,
    BLEND_OP_SUBTRACT,
    BLEND_OP_MIN,
    BLEND_OP_MAX
};

struct blend_desc
{
    bool enable;

    blend_factor source_factor;
    blend_factor target_factor;
    blend_op op;

    blend_factor source_alpha_factor;
    blend_factor target_alpha_factor;
    blend_op alpha_op;
};

enum depth_functor
{
    DEPTH_FUNCTOR_NEVER,
    DEPTH_FUNCTOR_LESS,
    DEPTH_FUNCTOR_EQUAL,
    DEPTH_FUNCTOR_LESS_EQUAL,
    DEPTH_FUNCTOR_GREATER,
    DEPTH_FUNCTOR_NOT_EQUAL,
    DEPTH_FUNCTOR_GREATER_EQUAL,
    DEPTH_FUNCTOR_ALWAYS
};

struct depth_stencil_desc
{
    depth_functor depth_functor;
};

enum cull_mode
{
    CULL_MODE_NONE,
    CULL_MODE_FRONT,
    CULL_MODE_BACK
};

struct rasterizer_desc
{
    cull_mode cull_mode;
};

enum primitive_topology_type
{
    PRIMITIVE_TOPOLOGY_TYPE_LINE,
    PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE
};

enum attachment_reference_type
{
    ATTACHMENT_REFERENCE_TYPE_UNUSE,
    ATTACHMENT_REFERENCE_TYPE_INPUT,
    ATTACHMENT_REFERENCE_TYPE_COLOR,
    ATTACHMENT_REFERENCE_TYPE_DEPTH,
    ATTACHMENT_REFERENCE_TYPE_RESOLVE
};

struct attachment_reference
{
    attachment_reference_type type;
    std::size_t resolve_relation;
};

struct render_pass_desc
{
    const char* vertex_shader;
    const char* pixel_shader;

    vertex_attribute vertex_attributes[16];
    std::size_t vertex_attribute_count;

    pipeline_parameter_layout_interface* parameters[16];
    std::size_t parameter_count;

    blend_desc blend;
    depth_stencil_desc depth_stencil;
    rasterizer_desc rasterizer;

    std::size_t samples;

    primitive_topology_type primitive_topology;

    attachment_reference references[16];
    std::size_t reference_count;
};

enum attachment_load_op
{
    ATTACHMENT_LOAD_OP_LOAD,
    ATTACHMENT_LOAD_OP_CLEAR,
    ATTACHMENT_LOAD_OP_DONT_CARE
};

enum attachment_store_op
{
    ATTACHMENT_STORE_OP_STORE,
    ATTACHMENT_STORE_OP_DONT_CARE
};

enum attachment_type
{
    ATTACHMENT_TYPE_RENDER_TARGET,
    ATTACHMENT_TYPE_CAMERA_RENDER_TARGET,
    ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE,
    ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL
};

struct attachment_desc
{
    attachment_type type;
    resource_format format;

    attachment_load_op load_op;
    attachment_store_op store_op;
    attachment_load_op stencil_load_op;
    attachment_store_op stencil_store_op;

    resource_state initial_state;
    resource_state final_state;

    std::size_t samples;
};

struct render_pipeline_desc
{
    attachment_desc attachments[16];
    std::size_t attachment_count;

    render_pass_desc passes[16];
    std::size_t pass_count;
};

class render_pipeline_interface
{
public:
    virtual ~render_pipeline_interface() = default;
};

struct compute_pipeline_desc
{
    const char* compute_shader;
    pipeline_parameter_layout_interface* parameters[16];
    std::size_t parameter_count;
};

class compute_pipeline_interface
{
public:
    virtual ~compute_pipeline_interface() = default;
};

struct scissor_extent
{
    std::uint32_t min_x;
    std::uint32_t min_y;
    std::uint32_t max_x;
    std::uint32_t max_y;
};

enum primitive_topology
{
    PRIMITIVE_TOPOLOGY_LINE_LIST,
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
};

class render_command_interface
{
public:
    virtual ~render_command_interface() = default;

    // Render.
    virtual void begin(
        render_pipeline_interface* pipeline,
        resource_interface* render_target,
        resource_interface* render_target_resolve,
        resource_interface* depth_stencil_buffer) = 0;
    virtual void end(render_pipeline_interface* pipeline) = 0;
    virtual void next_pass(render_pipeline_interface* pipeline) = 0;

    virtual void scissor(const scissor_extent* extents, std::size_t size) = 0;

    virtual void parameter(std::size_t i, pipeline_parameter_interface*) = 0;

    virtual void input_assembly_state(
        resource_interface* const* vertex_buffers,
        std::size_t vertex_buffer_count,
        resource_interface* index_buffer,
        primitive_topology primitive_topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) = 0;
    virtual void draw(std::size_t vertex_start, std::size_t vertex_end) = 0;
    virtual void draw_indexed(
        std::size_t index_start,
        std::size_t index_end,
        std::size_t vertex_base) = 0;

    virtual void clear_render_target(
        resource_interface* render_target,
        const math::float4& color) = 0;
    virtual void clear_depth_stencil(resource_interface* depth_stencil) = 0;

    // Compute.
    virtual void begin(compute_pipeline_interface* pipeline) {}
    virtual void end(compute_pipeline_interface* pipeline) {}
    virtual void dispatch(std::size_t x, std::size_t y, std::size_t z) {}
    virtual void compute_parameter(std::size_t index, pipeline_parameter_interface* parameter) {}
};

class renderer_interface
{
public:
    virtual ~renderer_interface() = default;

    virtual void present() = 0;

    virtual render_command_interface* allocate_command() = 0;
    virtual void execute(render_command_interface* command) = 0;

    virtual resource_interface* back_buffer() = 0;

    virtual void resize(std::uint32_t width, std::uint32_t height) = 0;
};

enum vertex_buffer_flags
{
    VERTEX_BUFFER_FLAG_NONE = 0,
    VERTEX_BUFFER_FLAG_COMPUTE_IN = 0x1,
    VERTEX_BUFFER_FLAG_COMPUTE_OUT = 0x2
};

struct vertex_buffer_desc
{
    const void* vertices;
    std::size_t vertex_size;
    std::size_t vertex_count;
    vertex_buffer_flags flags;
    bool dynamic;
    bool frame_resource;
};

struct index_buffer_desc
{
    const void* indices;
    std::size_t index_size;
    std::size_t index_count;
    bool dynamic;
    bool frame_resource;
};

struct render_target_desc
{
    std::uint32_t width;
    std::uint32_t height;
    std::size_t samples;
    resource_format format;
};

struct depth_stencil_buffer_desc
{
    std::uint32_t width;
    std::uint32_t height;
    std::size_t samples;
    resource_format format;
};

struct rhi_desc
{
    std::uint32_t width;
    std::uint32_t height;

    void* window_handle;

    std::size_t frame_resource;
    std::size_t render_concurrency;
};

class rhi_interface
{
public:
    virtual ~rhi_interface() = default;

    virtual void initialize(const rhi_desc& desc) = 0;

    virtual renderer_interface* make_renderer() = 0;

    virtual render_pipeline_interface* make_render_pipeline(const render_pipeline_desc& desc) = 0;
    virtual compute_pipeline_interface* make_compute_pipeline(
        const compute_pipeline_desc& desc) = 0;

    virtual pipeline_parameter_layout_interface* make_pipeline_parameter_layout(
        const pipeline_parameter_layout_desc& desc) = 0;
    virtual pipeline_parameter_interface* make_pipeline_parameter(
        pipeline_parameter_layout_interface* layout) = 0;

    virtual resource_interface* make_vertex_buffer(const vertex_buffer_desc& desc) = 0;
    virtual resource_interface* make_index_buffer(const index_buffer_desc& desc) = 0;

    virtual resource_interface* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format = RESOURCE_FORMAT_R8G8B8A8_UNORM) = 0;
    virtual resource_interface* make_texture(const char* file) = 0;
    virtual resource_interface* make_texture_cube(
        const char* left,
        const char* right,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back) = 0;

    virtual resource_interface* make_render_target(const render_target_desc& desc) = 0;
    virtual resource_interface* make_depth_stencil_buffer(
        const depth_stencil_buffer_desc& desc) = 0;
};
using make_rhi = rhi_interface* (*)();
} // namespace ash::graphics
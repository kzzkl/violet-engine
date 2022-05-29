#pragma once

#include "math/math.hpp"
#include "plugin_interface.hpp"
#include <cstddef>
#include <cstdint>

namespace ash::graphics
{
enum class resource_format
{
    UNDEFINED,

    R8_UNORM,
    R8_UINT,

    R8G8B8A8_UNORM,
    B8G8R8A8_UNORM,

    R32G32B32A32_FLOAT,
    R32G32B32A32_SINT,
    R32G32B32A32_UINT,

    D24_UNORM_S8_UINT
};

enum class resource_state
{
    UNDEFINED,
    RENDER_TARGET,
    DEPTH_STENCIL,
    PRESENT
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
using resource = resource_interface;

enum class vertex_attribute_type : std::uint8_t
{
    INT,    // R32 INT
    INT2,   // R32G32 INT
    INT3,   // R32G32B32 INT
    INT4,   // R32G32B32A32 INT
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

struct vertex_attribute
{
    const char* name;
    vertex_attribute_type type;
};

enum class pipeline_parameter_type
{
    BOOL,
    UINT,
    FLOAT,
    FLOAT2,
    FLOAT3,
    FLOAT4,
    FLOAT4x4,
    FLOAT4x4_ARRAY,
    SHADER_RESOURCE,
    UNORDERED_ACCESS
};

struct pipeline_parameter_pair
{
    pipeline_parameter_type type;
    std::size_t size; // Only for array.
};

struct pipeline_parameter_layout_desc
{
    pipeline_parameter_pair* parameters;
    std::size_t size;
};

class pipeline_parameter_layout_interface
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

    virtual void reset() = 0;
};

enum class blend_factor
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

enum class blend_op
{
    ADD,
    SUBTRACT,
    MIN,
    MAX
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

enum class depth_functor
{
    NEVER,
    LESS,
    EQUAL,
    LESS_EQUAL,
    GREATER,
    NOT_EQUAL,
    GREATER_EQUAL,
    ALWAYS
};

struct depth_stencil_desc
{
    depth_functor depth_functor;
};

enum class cull_mode
{
    NONE,
    FRONT,
    BACK
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

enum class attachment_reference_type
{
    UNUSE,
    INPUT,
    COLOR,
    DEPTH,
    RESOLVE
};

struct attachment_reference
{
    attachment_reference_type type;
    std::size_t resolve_relation;
};

struct pipeline_desc
{
    const char* vertex_shader;
    const char* pixel_shader;

    vertex_attribute* vertex_attributes;
    std::size_t vertex_attribute_count;

    pipeline_parameter_layout_interface** parameters;
    std::size_t parameter_count;

    blend_desc blend;
    depth_stencil_desc depth_stencil;
    rasterizer_desc rasterizer;

    std::size_t samples;

    primitive_topology_type primitive_topology;

    attachment_reference* references;
    std::size_t reference_count;
};

enum class attachment_load_op
{
    LOAD,
    CLEAR,
    DONT_CARE
};

enum class attachment_store_op
{
    STORE,
    DONT_CARE
};

enum class attachment_type
{
    RENDER_TARGET,
    CAMERA_RENDER_TARGET,
    CAMERA_RENDER_TARGET_RESOLVE,
    CAMERA_DEPTH_STENCIL
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

struct render_pass_desc
{
    attachment_desc* attachments;
    std::size_t attachment_count;

    pipeline_desc* subpasses;
    std::size_t subpass_count;
};

class render_pass_interface
{
public:
    virtual ~render_pass_interface() = default;
};

struct compute_pipeline_desc
{
    const char* compute_shader;
    pipeline_parameter_layout_interface** parameters;
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
    // Graphics.
    virtual void begin(
        render_pass_interface* render_pass,
        resource_interface* render_target,
        resource_interface* render_target_resolve,
        resource_interface* depth_stencil_buffer) = 0;
    virtual void end(render_pass_interface* render_pass) = 0;
    virtual void next(render_pass_interface* render_pass) = 0;

    virtual void scissor(const scissor_extent* extents, std::size_t size) = 0;

    virtual void parameter(std::size_t i, pipeline_parameter_interface*) = 0;
    virtual void draw(
        resource_interface* const* vertex_buffers,
        std::size_t vertex_buffer_count,
        resource_interface* index_buffer,
        std::size_t index_start,
        std::size_t index_end,
        std::size_t vertex_base,
        primitive_topology primitive_topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) = 0;

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

struct renderer_desc
{
    std::uint32_t width;
    std::uint32_t height;

    void* window_handle;

    std::size_t frame_resource;
    std::size_t render_concurrency;
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
using renderer = renderer_interface;

enum vertex_buffer_flags
{
    VERTEX_BUFFER_FLAG_NONE = 0,
    VERTEX_BUFFER_FLAG_SKIN_IN = 0x1,
    VERTEX_BUFFER_FLAG_SKIN_OUT = 0x2
};

struct vertex_buffer_desc
{
    const void* vertices;
    std::size_t vertex_size;
    std::size_t vertex_count;
    vertex_buffer_flags flags;
    bool dynamic;
};

struct index_buffer_desc
{
    const void* indices;
    std::size_t index_size;
    std::size_t index_count;
    bool dynamic;
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

class factory_interface
{
public:
    virtual ~factory_interface() = default;

    virtual renderer_interface* make_renderer(const renderer_desc& desc) = 0;

    virtual render_pass_interface* make_render_pass(const render_pass_desc& desc) = 0;
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
        resource_format format = resource_format::R8G8B8A8_UNORM) = 0;
    virtual resource_interface* make_texture(const char* file) = 0;
    virtual resource_interface* make_render_target(const render_target_desc& desc) = 0;
    virtual resource_interface* make_depth_stencil_buffer(
        const depth_stencil_buffer_desc& desc) = 0;
};
using factory = factory_interface;

using make_factory = factory_interface* (*)();
} // namespace ash::graphics
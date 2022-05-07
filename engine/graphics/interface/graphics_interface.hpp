#pragma once

#include "math.hpp"
#include "plugin_interface.hpp"
#include <cstddef>
#include <cstdint>

namespace ash::graphics
{
enum class resource_format
{
    UNDEFINED,

    R8G8B8A8_UNORM,
    B8G8R8A8_UNORM,

    R32G32B32A32_FLOAT,
    R32G32B32A32_INT,
    R32G32B32A32_UINT,
    D24_UNORM_S8_UINT
};

enum class resource_state
{
    UNDEFINED,
    PRESENT,
    DEPTH_STENCIL
};

class resource_interface
{
public:
    virtual ~resource_interface() = default;

    virtual resource_format format() const noexcept = 0;
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
    TEXTURE
};

struct pipeline_parameter_pair
{
    pipeline_parameter_type type;
    std::size_t size; // Only for array.
};

struct pipeline_layout_desc
{
    pipeline_parameter_pair* parameters;
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
    LESS,
    ALWAYS
};

struct depth_stencil_desc
{
    depth_functor depth_functor;
};

enum class primitive_topology
{
    TRIANGLE_LIST,
    LINE_LIST
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

    vertex_attribute_type* vertex_attributes;
    std::size_t vertex_attribute_count;

    pipeline_layout_interface** parameters;
    std::size_t parameter_count;

    blend_desc blend;
    depth_stencil_desc depth_stencil;

    std::size_t samples;

    primitive_topology primitive_topology;

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

struct attachment_desc
{
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
};

struct attachment_set_desc
{
    resource_interface** attachments;
    std::size_t attachment_count;

    std::uint32_t width;
    std::uint32_t height;

    render_pass_interface* render_pass;
};

class attachment_set_interface
{
};

struct scissor_rect
{
    std::uint32_t min_x;
    std::uint32_t min_y;
    std::uint32_t max_x;
    std::uint32_t max_y;
};

class render_command_interface
{
public:
    virtual void begin(
        render_pass_interface* render_pass,
        attachment_set_interface* attachment_set) = 0;
    virtual void end(render_pass_interface* render_pass) = 0;
    virtual void next(render_pass_interface* render_pass) = 0;

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

    std::size_t frame_resource;
    std::size_t render_concurrency;
};

class renderer_interface
{
public:
    virtual std::size_t begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual render_command_interface* allocate_command() = 0;
    virtual void execute(render_command_interface* command) = 0;

    virtual resource_interface* back_buffer(std::size_t index) = 0;
    virtual std::size_t back_buffer_count() = 0;

    virtual void resize(std::uint32_t width, std::uint32_t height) {}
};
using renderer = renderer_interface;

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
    virtual renderer_interface* make_renderer(const renderer_desc& desc) = 0;

    virtual attachment_set_interface* make_attachment_set(const attachment_set_desc& desc) = 0;
    virtual render_pass_interface* make_render_pass(const render_pass_desc& desc) = 0;

    virtual pipeline_layout_interface* make_pipeline_layout(const pipeline_layout_desc& desc) = 0;
    virtual pipeline_parameter_interface* make_pipeline_parameter(
        pipeline_layout_interface* layout) = 0;

    virtual resource_interface* make_vertex_buffer(const vertex_buffer_desc& desc) = 0;
    virtual resource_interface* make_index_buffer(const index_buffer_desc& desc) = 0;
    virtual resource_interface* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height) = 0;
    virtual resource_interface* make_texture(const char* file) = 0;
    virtual resource_interface* make_render_target(const render_target_desc& desc) = 0;
    virtual resource_interface* make_depth_stencil_buffer(
        const depth_stencil_buffer_desc& desc) = 0;
};
using factory = factory_interface;

using make_factory = factory_interface* (*)();
} // namespace ash::graphics
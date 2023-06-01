#pragma once

#include "core/plugin/plugin_interface.hpp"
#include "math/math.hpp"
#include <cstddef>
#include <cstdint>

namespace violet
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
    RESOURCE_FORMAT_D24_UNORM_S8_UINT,
    RESOURCE_FORMAT_D32_FLOAT
};

enum resource_state
{
    RESOURCE_STATE_UNDEFINED,
    RESOURCE_STATE_SHADER_RESOURCE,
    RESOURCE_STATE_RENDER_TARGET,
    RESOURCE_STATE_DEPTH_READ,
    RESOURCE_STATE_DEPTH_WRITE,
    RESOURCE_STATE_PRESENT
};

struct resource_extent
{
    std::uint32_t width;
    std::uint32_t height;
};

class rhi_resource
{
public:
    virtual ~rhi_resource() = default;

    virtual resource_format get_format() const noexcept = 0;
    virtual resource_extent get_extent() const noexcept = 0;

    virtual void* get_buffer() { return nullptr; }
    virtual std::size_t get_buffer_size() const noexcept = 0;

    virtual void upload(const void* data, std::size_t size, std::size_t offset = 0) {}
};

enum render_attachment_load_op
{
    RENDER_ATTACHMENT_LOAD_OP_LOAD,
    RENDER_ATTACHMENT_LOAD_OP_CLEAR,
    RENDER_ATTACHMENT_LOAD_OP_DONT_CARE
};

enum render_attachment_store_op
{
    RENDER_ATTACHMENT_STORE_OP_STORE,
    RENDER_ATTACHMENT_STORE_OP_DONT_CARE
};

enum render_attachment_type
{
    RENDER_ATTACHMENT_TYPE_RENDER_TARGET,
    RENDER_ATTACHMENT_TYPE_CAMERA_RENDER_TARGET,
    RENDER_ATTACHMENT_TYPE_CAMERA_RENDER_TARGET_RESOLVE,
    RENDER_ATTACHMENT_TYPE_CAMERA_DEPTH_STENCIL
};

struct render_attachment_desc
{
    render_attachment_type type;
    resource_format format;

    render_attachment_load_op load_op;
    render_attachment_store_op store_op;
    render_attachment_load_op stencil_load_op;
    render_attachment_store_op stencil_store_op;

    resource_state initial_state;
    resource_state final_state;

    std::size_t samples;
};

enum render_attachment_reference_type
{
    RENDER_ATTACHMENT_REFERENCE_TYPE_INPUT,
    RENDER_ATTACHMENT_REFERENCE_TYPE_COLOR,
    RENDER_ATTACHMENT_REFERENCE_TYPE_DEPTH,
    RENDER_ATTACHMENT_REFERENCE_TYPE_RESOLVE
};

struct render_attachment_reference
{
    render_attachment_reference_type type;
    std::size_t index;
    std::size_t resolve_index;
};

struct render_subpass_desc
{
    render_attachment_reference references[16];
    std::size_t reference_count = 0;
};

struct render_pass_desc
{
    render_attachment_desc attachments[16];
    std::size_t attachment_count = 0;

    render_subpass_desc subpasses[16];
    std::size_t pass_count = 0;
};

class rhi_render_pass
{
public:
    virtual ~rhi_render_pass() = default;
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
    std::size_t size = 0;
};

struct pipeline_parameter_desc
{
    pipeline_parameter_pair parameters[16];
    std::size_t parameter_count = 0;
};

class rhi_pipeline_parameter
{
public:
    virtual ~rhi_pipeline_parameter() = default;

    virtual void set(std::size_t index, const void* data, size_t size) = 0;
    virtual void set(std::size_t index, rhi_resource* texture) = 0;

    virtual void* get_constant_buffer_pointer(std::size_t index) = 0;
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
    bool enable = false;

    blend_factor source_factor;
    blend_factor target_factor;
    blend_op op;

    blend_factor source_alpha_factor;
    blend_factor target_alpha_factor;
    blend_op alpha_op;
};

enum depth_stencil_functor
{
    DEPTH_STENCIL_FUNCTOR_NEVER,
    DEPTH_STENCIL_FUNCTOR_LESS,
    DEPTH_STENCIL_FUNCTOR_EQUAL,
    DEPTH_STENCIL_FUNCTOR_LESS_EQUAL,
    DEPTH_STENCIL_FUNCTOR_GREATER,
    DEPTH_STENCIL_FUNCTOR_NOT_EQUAL,
    DEPTH_STENCIL_FUNCTOR_GREATER_EQUAL,
    DEPTH_STENCIL_FUNCTOR_ALWAYS
};

enum stencil_op
{
    STENCIL_OP_KEEP,
    STENCIL_OP_ZERO,
    STENCIL_OP_REPLACE,
    STENCIL_OP_INCR_SAT,
    STENCIL_OP_DECR_SAT,
    STENCIL_OP_INVERT,
    STENCIL_OP_INCR,
    STENCIL_OP_DECR
};

struct depth_stencil_desc
{
    bool depth_enable = true;
    depth_stencil_functor depth_functor = DEPTH_STENCIL_FUNCTOR_LESS;

    bool stencil_enable = false;
    depth_stencil_functor stencil_functor = DEPTH_STENCIL_FUNCTOR_ALWAYS;
    stencil_op stencil_pass_op = STENCIL_OP_KEEP;
    stencil_op stencil_fail_op = STENCIL_OP_KEEP;
};

enum cull_mode
{
    CULL_MODE_NONE,
    CULL_MODE_FRONT,
    CULL_MODE_BACK
};

struct rasterizer_desc
{
    cull_mode cull_mode = CULL_MODE_BACK;
};

enum primitive_topology
{
    PRIMITIVE_TOPOLOGY_LINE_LIST,
    PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
};

struct render_pipeline_desc
{
    const char* vertex_shader = nullptr;
    const char* pixel_shader = nullptr;

    vertex_attribute* vertex_attributes;
    std::size_t vertex_attribute_count = 0;

    pipeline_parameter_desc* parameters;
    std::size_t parameter_count = 0;

    blend_desc blend;
    depth_stencil_desc depth_stencil;
    rasterizer_desc rasterizer;

    std::size_t samples;

    primitive_topology primitive_topology;

    rhi_render_pass* render_pass;
    std::size_t render_subpass_index;
};

class rhi_render_pipeline
{
public:
    virtual ~rhi_render_pipeline() = default;
};

struct scissor_extent
{
    std::uint32_t min_x;
    std::uint32_t min_y;
    std::uint32_t max_x;
    std::uint32_t max_y;
};

class rhi_render_command
{
public:
    virtual ~rhi_render_command() = default;

    virtual void begin(
        rhi_render_pass* render_pass,
        rhi_resource* const* attachments,
        std::size_t attachment_count) = 0;
    virtual void end() = 0;
    virtual void next() = 0;

    virtual void set_pipeline(rhi_render_pipeline* render_pipeline) = 0;
    virtual void set_parameter(std::size_t index, rhi_pipeline_parameter* parameter) = 0;

    virtual void set_viewport(
        float x,
        float y,
        float width,
        float height,
        float min_depth,
        float max_depth) = 0;
    virtual void set_scissor(const scissor_extent* extents, std::size_t size) = 0;

    virtual void set_input_assembly_state(
        rhi_resource* const* vertex_buffers,
        std::size_t vertex_buffer_count,
        rhi_resource* index_buffer,
        primitive_topology primitive_topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) = 0;

    virtual void draw(std::size_t vertex_start, std::size_t vertex_end) = 0;
    virtual void draw_indexed(
        std::size_t index_start,
        std::size_t index_end,
        std::size_t vertex_base) = 0;

    virtual void clear_render_target(rhi_resource* render_target, const float4& color) = 0;
    virtual void clear_depth_stencil(
        rhi_resource* depth_stencil,
        bool clear_depth = true,
        float depth = 1.0f,
        bool clear_stencil = true,
        std::uint8_t stencil = 0) = 0;
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
    std::size_t vertex_count = 0;
    vertex_buffer_flags flags;
    bool dynamic;
    bool frame_resource;
};

struct index_buffer_desc
{
    const void* indices;
    std::size_t index_size;
    std::size_t index_count = 0;
    bool dynamic;
    bool frame_resource;
};

struct shadow_map_desc
{
    std::uint32_t width;
    std::uint32_t height;
    std::size_t samples;
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

class rhi_context
{
public:
    virtual ~rhi_context() = default;

    virtual bool initialize(const rhi_desc& desc) = 0;

    virtual rhi_render_command* allocate_command() = 0;
    virtual void execute(rhi_render_command* command) = 0;

    virtual void present() = 0;
    virtual void resize(std::uint32_t width, std::uint32_t height) = 0;

    virtual rhi_resource* get_back_buffer() = 0;

    virtual rhi_render_pipeline* make_render_pipeline(const render_pipeline_desc& desc) = 0;

    virtual rhi_pipeline_parameter* make_pipeline_parameter(
        const pipeline_parameter_desc& desc) = 0;

    virtual rhi_resource* make_vertex_buffer(const vertex_buffer_desc& desc) = 0;
    virtual rhi_resource* make_index_buffer(const index_buffer_desc& desc) = 0;

    virtual rhi_resource* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        resource_format format = RESOURCE_FORMAT_R8G8B8A8_UNORM) = 0;
    virtual rhi_resource* make_texture(const char* file) = 0;
    virtual rhi_resource* make_texture_cube(
        const char* left,
        const char* right,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back) = 0;

    virtual rhi_resource* make_shadow_map(const shadow_map_desc& desc) = 0;

    virtual rhi_resource* make_render_target(const render_target_desc& desc) = 0;
    virtual rhi_resource* make_depth_stencil_buffer(const depth_stencil_buffer_desc& desc) = 0;
};
using make_rhi = rhi_context* (*)();
using destroy_rhi = void (*)(rhi_context*);
} // namespace violet
#pragma once

#include "core/plugin_interface.hpp"
#include "math/math.hpp"
#include <cstddef>
#include <cstdint>

namespace violet
{
enum rhi_resource_format
{
    RHI_RESOURCE_FORMAT_UNDEFINED,
    RHI_RESOURCE_FORMAT_R8_UNORM,
    RHI_RESOURCE_FORMAT_R8_SNORM,
    RHI_RESOURCE_FORMAT_R8_UINT,
    RHI_RESOURCE_FORMAT_R8_SINT,
    RHI_RESOURCE_FORMAT_R8G8_UNORM,
    RHI_RESOURCE_FORMAT_R8G8_SNORM,
    RHI_RESOURCE_FORMAT_R8G8_UINT,
    RHI_RESOURCE_FORMAT_R8G8_SINT,
    RHI_RESOURCE_FORMAT_R8G8B8_UNORM,
    RHI_RESOURCE_FORMAT_R8G8B8_SNORM,
    RHI_RESOURCE_FORMAT_R8G8B8_UINT,
    RHI_RESOURCE_FORMAT_R8G8B8_SINT,
    RHI_RESOURCE_FORMAT_R8G8B8A8_UNORM,
    RHI_RESOURCE_FORMAT_R8G8B8A8_SNORM,
    RHI_RESOURCE_FORMAT_R8G8B8A8_UINT,
    RHI_RESOURCE_FORMAT_R8G8B8A8_SINT,
    RHI_RESOURCE_FORMAT_B8G8R8A8_UNORM,
    RHI_RESOURCE_FORMAT_B8G8R8A8_SNORM,
    RHI_RESOURCE_FORMAT_B8G8R8A8_UINT,
    RHI_RESOURCE_FORMAT_B8G8R8A8_SINT,
    RHI_RESOURCE_FORMAT_B8G8R8A8_SRGB,
    RHI_RESOURCE_FORMAT_R32_UINT,
    RHI_RESOURCE_FORMAT_R32_SINT,
    RHI_RESOURCE_FORMAT_R32_FLOAT,
    RHI_RESOURCE_FORMAT_R32G32_UINT,
    RHI_RESOURCE_FORMAT_R32G32_SINT,
    RHI_RESOURCE_FORMAT_R32G32_FLOAT,
    RHI_RESOURCE_FORMAT_R32G32B32_UINT,
    RHI_RESOURCE_FORMAT_R32G32B32_SINT,
    RHI_RESOURCE_FORMAT_R32G32B32_FLOAT,
    RHI_RESOURCE_FORMAT_R32G32B32A32_UINT,
    RHI_RESOURCE_FORMAT_R32G32B32A32_SINT,
    RHI_RESOURCE_FORMAT_R32G32B32A32_FLOAT,
    RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT,
    RHI_RESOURCE_FORMAT_D32_FLOAT
};

enum rhi_resource_state
{
    RHI_RESOURCE_STATE_UNDEFINED,
    RHI_RESOURCE_STATE_SHADER_RESOURCE,
    RHI_RESOURCE_STATE_RENDER_TARGET,
    RHI_RESOURCE_STATE_DEPTH_READ,
    RHI_RESOURCE_STATE_DEPTH_WRITE,
    RHI_RESOURCE_STATE_PRESENT
};

struct rhi_resource_extent
{
    std::uint32_t width;
    std::uint32_t height;
};

enum rhi_sample_count
{
    RHI_SAMPLE_COUNT_1,
    RHI_SAMPLE_COUNT_2,
    RHI_SAMPLE_COUNT_4,
    RHI_SAMPLE_COUNT_8,
    RHI_SAMPLE_COUNT_16,
    RHI_SAMPLE_COUNT_32,
};

class rhi_resource
{
public:
    virtual ~rhi_resource() = default;

    // For texture.
    virtual rhi_resource_format get_format() const noexcept = 0;
    virtual rhi_resource_extent get_extent() const noexcept = 0;

    // For buffer.
    virtual void* get_buffer() { return nullptr; }
    virtual std::size_t get_buffer_size() const noexcept = 0;

    virtual std::size_t get_hash() const noexcept = 0;
};

enum rhi_attachment_load_op
{
    RHI_ATTACHMENT_LOAD_OP_LOAD,
    RHI_ATTACHMENT_LOAD_OP_CLEAR,
    RHI_ATTACHMENT_LOAD_OP_DONT_CARE
};

enum rhi_attachment_store_op
{
    RHI_ATTACHMENT_STORE_OP_STORE,
    RHI_ATTACHMENT_STORE_OP_DONT_CARE
};

struct rhi_attachment_desc
{
    rhi_resource_format format;

    rhi_attachment_load_op load_op;
    rhi_attachment_store_op store_op;
    rhi_attachment_load_op stencil_load_op;
    rhi_attachment_store_op stencil_store_op;

    rhi_resource_state initial_state;
    rhi_resource_state final_state;

    rhi_sample_count samples;
};

enum rhi_attachment_reference_type
{
    RHI_ATTACHMENT_REFERENCE_TYPE_INPUT,
    RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
    RHI_ATTACHMENT_REFERENCE_TYPE_RESOLVE
};

enum rhi_attachment_layout
{
    RHI_ATTACHMENT_LAYOUT_OPTIMAL
};

struct rhi_attachment_reference
{
    rhi_attachment_reference_type type;
    rhi_resource_state state;
    std::size_t index;
    std::size_t resolve_index;
};

struct rhi_render_subpass_desc
{
    rhi_attachment_reference references[16];
    std::size_t reference_count = 0;
};

struct rhi_render_pass_desc
{
    rhi_attachment_desc attachments[16];
    std::size_t attachment_count = 0;

    rhi_render_subpass_desc subpasses[16];
    std::size_t subpass_count = 0;
};

class rhi_render_pass
{
public:
    virtual ~rhi_render_pass() = default;
};

enum rhi_pipeline_parameter_type
{
    RHI_PIPELINE_PARAMETER_TYPE_UNIFORM_BUFFER,
    RHI_PIPELINE_PARAMETER_TYPE_SHADER_RESOURCE,
    RHI_PIPELINE_PARAMETER_TYPE_UNORDERED_ACCESS
};

struct rhi_pipeline_parameter_layout_pair
{
    rhi_pipeline_parameter_type type;
    std::size_t size = 0;
};

struct rhi_pipeline_parameter_layout_desc
{
    rhi_pipeline_parameter_layout_pair parameters[16];
    std::size_t parameter_count = 0;
};

class rhi_pipeline_parameter_layout
{
public:
    virtual ~rhi_pipeline_parameter_layout() = default;
};

class rhi_pipeline_parameter
{
public:
    virtual ~rhi_pipeline_parameter() = default;

    virtual void set(std::size_t index, const void* data, std::size_t size, std::size_t offset) = 0;
    virtual void set(std::size_t index, rhi_resource* texture) = 0;
};

struct rhi_vertex_attribute
{
    const char* name;
    rhi_resource_format format;
};

enum rhi_blend_factor
{
    RHI_BLEND_FACTOR_ZERO,
    RHI_BLEND_FACTOR_ONE,
    RHI_BLEND_FACTOR_SOURCE_COLOR,
    RHI_BLEND_FACTOR_SOURCE_ALPHA,
    RHI_BLEND_FACTOR_SOURCE_INV_ALPHA,
    RHI_BLEND_FACTOR_TARGET_COLOR,
    RHI_BLEND_FACTOR_TARGET_ALPHA,
    RHI_BLEND_FACTOR_TARGET_INV_ALPHA
};

enum rhi_blend_op
{
    RHI_BLEND_OP_ADD,
    RHI_BLEND_OP_SUBTRACT,
    RHI_BLEND_OP_MIN,
    RHI_BLEND_OP_MAX
};

struct rhi_blend_desc
{
    bool enable = false;

    rhi_blend_factor source_factor;
    rhi_blend_factor target_factor;
    rhi_blend_op op;

    rhi_blend_factor source_alpha_factor;
    rhi_blend_factor target_alpha_factor;
    rhi_blend_op alpha_op;
};

enum rhi_depth_stencil_functor
{
    RHI_DEPTH_STENCIL_FUNCTOR_NEVER,
    RHI_DEPTH_STENCIL_FUNCTOR_LESS,
    RHI_DEPTH_STENCIL_FUNCTOR_EQUAL,
    RHI_DEPTH_STENCIL_FUNCTOR_LESS_EQUAL,
    RHI_DEPTH_STENCIL_FUNCTOR_GREATER,
    RHI_DEPTH_STENCIL_FUNCTOR_NOT_EQUAL,
    RHI_DEPTH_STENCIL_FUNCTOR_GREATER_EQUAL,
    RHI_DEPTH_STENCIL_FUNCTOR_ALWAYS
};

enum rhi_stencil_op
{
    RHI_STENCIL_OP_KEEP,
    RHI_STENCIL_OP_ZERO,
    RHI_STENCIL_OP_REPLACE,
    RHI_STENCIL_OP_INCR_SAT,
    RHI_STENCIL_OP_DECR_SAT,
    RHI_STENCIL_OP_INVERT,
    RHI_STENCIL_OP_INCR,
    RHI_STENCIL_OP_DECR
};

struct rhi_depth_stencil_desc
{
    bool depth_enable = true;
    rhi_depth_stencil_functor depth_functor = RHI_DEPTH_STENCIL_FUNCTOR_LESS;

    bool stencil_enable = false;
    rhi_depth_stencil_functor stencil_functor = RHI_DEPTH_STENCIL_FUNCTOR_ALWAYS;
    rhi_stencil_op stencil_pass_op = RHI_STENCIL_OP_KEEP;
    rhi_stencil_op stencil_fail_op = RHI_STENCIL_OP_KEEP;
};

enum rhi_cull_mode
{
    RHI_CULL_MODE_NONE,
    RHI_CULL_MODE_FRONT,
    RHI_CULL_MODE_BACK
};

struct rhi_rasterizer_desc
{
    rhi_cull_mode cull_mode = RHI_CULL_MODE_BACK;
};

enum rhi_primitive_topology
{
    RHI_PRIMITIVE_TOPOLOGY_LINE_LIST,
    RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
};

struct rhi_render_pipeline_desc
{
    const char* vertex_shader = nullptr;
    const char* pixel_shader = nullptr;

    rhi_vertex_attribute* vertex_attributes;
    std::size_t vertex_attribute_count = 0;

    rhi_pipeline_parameter_layout** parameters;
    std::size_t parameter_count = 0;

    rhi_blend_desc blend;
    rhi_depth_stencil_desc depth_stencil;
    rhi_rasterizer_desc rasterizer;

    rhi_sample_count samples;

    rhi_primitive_topology primitive_topology;

    rhi_render_pass* render_pass;
    std::size_t render_subpass_index;
};

class rhi_render_pipeline
{
public:
    virtual ~rhi_render_pipeline() = default;
};

struct rhi_framebuffer_desc
{
    rhi_render_pass* render_pass;
    const rhi_resource* attachments[32];
    std::size_t attachment_count;
};

class rhi_framebuffer
{
public:
    virtual ~rhi_framebuffer() = default;
};

struct rhi_viewport
{
    float x;
    float y;
    float width;
    float height;
    float min_depth;
    float max_depth;
};

struct rhi_scissor_rect
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

    virtual void begin(rhi_render_pass* render_pass, rhi_framebuffer* framebuffer) = 0;
    virtual void end() = 0;
    virtual void next() = 0;

    virtual void set_pipeline(rhi_render_pipeline* render_pipeline) = 0;
    virtual void set_parameter(std::size_t index, rhi_pipeline_parameter* pipeline_parameter) = 0;

    virtual void set_viewport(const rhi_viewport& viewport) = 0;
    virtual void set_scissor(const rhi_scissor_rect* rects, std::size_t size) = 0;

    virtual void set_vertex_buffers(
        rhi_resource* const* vertex_buffers,
        std::size_t vertex_buffer_count) = 0;
    virtual void set_index_buffer(rhi_resource* index_buffer) = 0;

    virtual void draw(std::size_t vertex_start, std::size_t vertex_count) = 0;
    virtual void draw_indexed(
        std::size_t index_start,
        std::size_t index_count,
        std::size_t vertex_base) = 0;

    virtual void clear_render_target(rhi_resource* render_target, const float4& color) = 0;
    virtual void clear_depth_stencil(
        rhi_resource* depth_stencil,
        bool clear_depth = true,
        float depth = 1.0f,
        bool clear_stencil = true,
        std::uint8_t stencil = 0) = 0;
};

class rhi_fence
{
public:
    virtual ~rhi_fence() = default;

    virtual void wait() = 0;
};

class rhi_semaphore
{
public:
    virtual ~rhi_semaphore() = default;
};

enum rhi_vertex_buffer_flags
{
    RHI_VERTEX_BUFFER_FLAG_NONE = 0,
    RHI_VERTEX_BUFFER_FLAG_COMPUTE_IN = 0x1,
    RHI_VERTEX_BUFFER_FLAG_COMPUTE_OUT = 0x2
};

struct rhi_vertex_buffer_desc
{
    const void* data;
    std::size_t size;

    rhi_vertex_buffer_flags flags;
    bool dynamic;
    bool frame_resource;
};

struct rhi_index_buffer_desc
{
    const void* data;
    std::size_t size;

    std::size_t index_size;

    bool dynamic;
    bool frame_resource;
};

struct rhi_shadow_map_desc
{
    std::uint32_t width;
    std::uint32_t height;
    rhi_sample_count samples;
};

struct rhi_render_target_desc
{
    std::uint32_t width;
    std::uint32_t height;
    rhi_sample_count samples;
    rhi_resource_format format;
};

struct rhi_depth_stencil_buffer_desc
{
    std::uint32_t width;
    std::uint32_t height;
    rhi_sample_count samples;
    rhi_resource_format format;
};

struct rhi_desc
{
    std::uint32_t width;
    std::uint32_t height;

    void* window_handle;

    std::size_t frame_resource_count;
    std::size_t render_concurrency;
};

class rhi_context
{
public:
    virtual ~rhi_context() = default;

    virtual bool initialize(const rhi_desc& desc) = 0;

    virtual rhi_render_command* allocate_command() = 0;
    virtual void execute(
        rhi_render_command* const* commands,
        std::size_t command_count,
        rhi_semaphore* const* signal_semaphores,
        std::size_t signal_semaphore_count,
        rhi_semaphore* const* wait_semaphores,
        std::size_t wait_semaphore_count,
        rhi_fence* fence) = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void present(
        rhi_semaphore* const* wait_semaphores,
        std::size_t wait_semaphore_count) = 0;

    virtual void resize(std::uint32_t width, std::uint32_t height) = 0;

    virtual rhi_resource* get_back_buffer() = 0;
    virtual rhi_fence* get_in_flight_fence() = 0;
    virtual rhi_semaphore* get_image_available_semaphore() = 0;

    virtual std::size_t get_frame_resource_count() const noexcept = 0;
    virtual std::size_t get_frame_resource_index() const noexcept = 0;

public:
    virtual rhi_render_pass* create_render_pass(const rhi_render_pass_desc& desc) = 0;
    virtual void destroy_render_pass(rhi_render_pass* render_pass) = 0;

    virtual rhi_render_pipeline* create_render_pipeline(const rhi_render_pipeline_desc& desc) = 0;
    virtual void destroy_render_pipeline(rhi_render_pipeline* render_pipeline) = 0;

    virtual rhi_pipeline_parameter_layout* create_pipeline_parameter_layout(
        const rhi_pipeline_parameter_layout_desc& desc) = 0;
    virtual void destroy_pipeline_parameter_layout(
        rhi_pipeline_parameter_layout* pipeline_parameter_layout) = 0;

    virtual rhi_pipeline_parameter* create_pipeline_parameter(
        rhi_pipeline_parameter_layout* layout) = 0;
    virtual void destroy_pipeline_parameter(rhi_pipeline_parameter* pipeline_parameter) = 0;

    virtual rhi_framebuffer* create_framebuffer(const rhi_framebuffer_desc& desc) = 0;
    virtual void destroy_framebuffer(rhi_framebuffer* framebuffer) = 0;

    virtual rhi_resource* create_vertex_buffer(const rhi_vertex_buffer_desc& desc) = 0;
    virtual void destroy_vertex_buffer(rhi_resource* vertex_buffer) = 0;

    virtual rhi_resource* create_index_buffer(const rhi_index_buffer_desc& desc) = 0;
    virtual void destroy_index_buffer(rhi_resource* index_buffer) = 0;

    virtual rhi_resource* create_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height,
        rhi_resource_format format = RHI_RESOURCE_FORMAT_R8G8B8A8_UNORM) = 0;
    virtual rhi_resource* create_texture(const char* file) = 0;
    virtual void destroy_texture(rhi_resource* texture) = 0;

    virtual rhi_resource* create_texture_cube(
        const char* left,
        const char* right,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back) = 0;

    virtual rhi_resource* create_shadow_map(const rhi_shadow_map_desc& desc) = 0;

    virtual rhi_resource* create_render_target(const rhi_render_target_desc& desc) = 0;
    virtual rhi_resource* create_depth_stencil_buffer(
        const rhi_depth_stencil_buffer_desc& desc) = 0;

    virtual rhi_fence* create_fence(bool signaled) = 0;
    virtual void destroy_fence(rhi_fence* fence) = 0;

    virtual rhi_semaphore* create_semaphore() = 0;
    virtual void destroy_semaphore(rhi_semaphore* semaphore) = 0;
};
using create_rhi = rhi_context* (*)();
using destroy_rhi = void (*)(rhi_context*);
} // namespace violet
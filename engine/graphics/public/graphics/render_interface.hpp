#pragma once

#include "core/plugin_interface.hpp"
#include <cstddef>
#include <cstdint>

namespace violet
{
enum rhi_format
{
    RHI_FORMAT_UNDEFINED,
    RHI_FORMAT_R8_UNORM,
    RHI_FORMAT_R8_SNORM,
    RHI_FORMAT_R8_UINT,
    RHI_FORMAT_R8_SINT,
    RHI_FORMAT_R8G8_UNORM,
    RHI_FORMAT_R8G8_SNORM,
    RHI_FORMAT_R8G8_UINT,
    RHI_FORMAT_R8G8_SINT,
    RHI_FORMAT_R8G8B8_UNORM,
    RHI_FORMAT_R8G8B8_SNORM,
    RHI_FORMAT_R8G8B8_UINT,
    RHI_FORMAT_R8G8B8_SINT,
    RHI_FORMAT_R8G8B8A8_UNORM,
    RHI_FORMAT_R8G8B8A8_SNORM,
    RHI_FORMAT_R8G8B8A8_UINT,
    RHI_FORMAT_R8G8B8A8_SINT,
    RHI_FORMAT_B8G8R8A8_UNORM,
    RHI_FORMAT_B8G8R8A8_SNORM,
    RHI_FORMAT_B8G8R8A8_UINT,
    RHI_FORMAT_B8G8R8A8_SINT,
    RHI_FORMAT_B8G8R8A8_SRGB,
    RHI_FORMAT_R32_UINT,
    RHI_FORMAT_R32_SINT,
    RHI_FORMAT_R32_FLOAT,
    RHI_FORMAT_R32G32_UINT,
    RHI_FORMAT_R32G32_SINT,
    RHI_FORMAT_R32G32_FLOAT,
    RHI_FORMAT_R32G32B32_UINT,
    RHI_FORMAT_R32G32B32_SINT,
    RHI_FORMAT_R32G32B32_FLOAT,
    RHI_FORMAT_R32G32B32A32_UINT,
    RHI_FORMAT_R32G32B32A32_SINT,
    RHI_FORMAT_R32G32B32A32_FLOAT,
    RHI_FORMAT_D24_UNORM_S8_UINT,
    RHI_FORMAT_D32_FLOAT
};

enum rhi_texture_layout
{
    RHI_TEXTURE_LAYOUT_UNDEFINED,
    RHI_TEXTURE_LAYOUT_GENERAL,
    RHI_TEXTURE_LAYOUT_SHADER_RESOURCE,
    RHI_TEXTURE_LAYOUT_RENDER_TARGET,
    RHI_TEXTURE_LAYOUT_DEPTH_STENCIL,
    RHI_TEXTURE_LAYOUT_PRESENT,
    RHI_TEXTURE_LAYOUT_TRANSFER_SRC,
    RHI_TEXTURE_LAYOUT_TRANSFER_DST
};

struct rhi_texture_extent
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

class rhi_texture
{
public:
    virtual ~rhi_texture() = default;

    virtual rhi_format get_format() const noexcept = 0;
    virtual rhi_texture_extent get_extent() const noexcept = 0;

    virtual std::size_t get_hash() const noexcept = 0;
};

class rhi_buffer
{
public:
    virtual ~rhi_buffer() = default;

    virtual void* get_buffer() { return nullptr; }
    virtual std::size_t get_buffer_size() const noexcept = 0;

    virtual std::size_t get_hash() const noexcept = 0;
};

enum rhi_filter
{
    RHI_FILTER_NEAREST,
    RHI_FILTER_LINEAR
};

enum rhi_sampler_address_mode
{
    RHI_SAMPLER_ADDRESS_MODE_REPEAT,
    RHI_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT,
    RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
    RHI_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
    RHI_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE
};

struct rhi_sampler_desc
{
    rhi_filter mag_filter;
    rhi_filter min_filter;

    rhi_sampler_address_mode address_mode_u;
    rhi_sampler_address_mode address_mode_v;
    rhi_sampler_address_mode address_mode_w;

    float min_mip_level;
    float max_mip_level;
};

class rhi_sampler
{
public:
    virtual ~rhi_sampler() = default;
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
    rhi_format format;
    rhi_sample_count samples;

    rhi_texture_layout initial_layout;
    rhi_texture_layout final_layout;

    rhi_attachment_load_op load_op;
    rhi_attachment_store_op store_op;
    rhi_attachment_load_op stencil_load_op;
    rhi_attachment_store_op stencil_store_op;
};

enum rhi_attachment_reference_type
{
    RHI_ATTACHMENT_REFERENCE_TYPE_INPUT,
    RHI_ATTACHMENT_REFERENCE_TYPE_COLOR,
    RHI_ATTACHMENT_REFERENCE_TYPE_DEPTH_STENCIL,
    RHI_ATTACHMENT_REFERENCE_TYPE_RESOLVE
};

struct rhi_attachment_reference
{
    rhi_attachment_reference_type type;
    rhi_texture_layout layout;
    std::size_t index;
    std::size_t resolve_index;
};

struct rhi_render_subpass_desc
{
    rhi_attachment_reference references[32];
    std::size_t reference_count = 0;
};

enum rhi_pipeline_stage_flag
{
    RHI_PIPELINE_STAGE_FLAG_BEGIN = 1 << 0,
    RHI_PIPELINE_STAGE_FLAG_VERTEX = 1 << 1,
    RHI_PIPELINE_STAGE_FLAG_EARLY_DEPTH_STENCIL = 1 << 2,
    RHI_PIPELINE_STAGE_FLAG_FRAGMENT = 1 << 3,
    RHI_PIPELINE_STAGE_FLAG_LATE_DEPTH_STENCIL = 1 << 4,
    RHI_PIPELINE_STAGE_FLAG_COLOR_OUTPUT = 1 << 5,
    RHI_PIPELINE_STAGE_FLAG_END = 1 << 6,
};
using rhi_pipeline_stage_flags = std::uint32_t;

enum rhi_access_flag
{
    RHI_ACCESS_FLAG_COLOR_READ = 1 << 0,
    RHI_ACCESS_FLAG_COLOR_WRITE = 1 << 1,
    RHI_ACCESS_FLAG_DEPTH_STENCIL_READ = 1 << 2,
    RHI_ACCESS_FLAG_DEPTH_STENCIL_WRITE = 1 << 3,
    RHI_ACCESS_FLAG_SHADER_READ = 1 << 0,
    RHI_ACCESS_FLAG_SHADER_WRITE = 1 << 1
};
using rhi_access_flags = std::uint32_t;

#define RHI_RENDER_SUBPASS_EXTERNAL (~0U)

struct rhi_render_subpass_dependency_desc
{
    std::size_t src;
    rhi_pipeline_stage_flags src_stage;
    rhi_access_flags src_access;

    std::size_t dst;
    rhi_pipeline_stage_flags dst_stage;
    rhi_access_flags dst_access;
};

struct rhi_render_pass_desc
{
    rhi_attachment_desc* attachments;
    std::size_t attachment_count = 0;

    rhi_render_subpass_desc* subpasses;
    std::size_t subpass_count = 0;

    rhi_render_subpass_dependency_desc* dependencies;
    std::size_t dependency_count;
};

class rhi_render_pass
{
public:
    virtual ~rhi_render_pass() = default;
};

enum rhi_parameter_type
{
    RHI_PARAMETER_TYPE_UNIFORM_BUFFER,
    RHI_PARAMETER_TYPE_STORAGE_BUFFER,
    RHI_PARAMETER_TYPE_TEXTURE
};

enum rhi_parameter_stage_flag
{
    RHI_PARAMETER_STAGE_FLAG_VERTEX = 1 << 0,
    RHI_PARAMETER_STAGE_FLAG_FRAGMENT = 1 << 1,
    RHI_PARAMETER_STAGE_FLAG_COMPUTE = 1 << 2
};
using rhi_parameter_stage_flags = std::uint32_t;

struct rhi_parameter_binding
{
    rhi_parameter_type type;
    rhi_parameter_stage_flags stage;
    std::size_t size = 0;
};

struct rhi_parameter_desc
{
    rhi_parameter_binding bindings[32];
    std::size_t binding_count = 0;
};

class rhi_parameter
{
public:
    virtual ~rhi_parameter() = default;

    virtual void set_uniform(
        std::size_t index,
        const void* data,
        std::size_t size,
        std::size_t offset) = 0;
    virtual void set_texture(std::size_t index, rhi_texture* texture, rhi_sampler* sampler) = 0;
    virtual void set_storage(std::size_t index, rhi_buffer* storage_buffer) = 0;
};

struct rhi_vertex_attribute
{
    const char* name;
    rhi_format format;
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

struct rhi_attachment_blend_desc
{
    bool enable = false;

    rhi_blend_factor src_factor;
    rhi_blend_factor dst_factor;
    rhi_blend_op op;

    rhi_blend_factor src_alpha_factor;
    rhi_blend_factor dst_alpha_factor;
    rhi_blend_op alpha_op;
};

struct rhi_blend_desc
{
    rhi_attachment_blend_desc attachments[32];
    std::size_t attachment_count;
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
    RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    RHI_PRIMITIVE_TOPOLOGY_LINE_LIST
};

class rhi_shader
{
public:
    virtual ~rhi_shader() = default;

    virtual const char* get_input_name(std::size_t index) = 0;
    virtual std::size_t get_input_location(std::size_t index) = 0;
    virtual rhi_format get_input_format(std::size_t index) = 0;
    virtual std::size_t get_input_count() const = 0;
};

struct rhi_render_pipeline_desc
{
    rhi_shader* vertex_shader;
    rhi_shader* fragment_shader;

    rhi_parameter_desc* parameters;
    std::size_t parameter_count;

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

struct rhi_compute_pipeline_desc
{
    rhi_shader* compute_shader;

    rhi_parameter_desc* parameters;
    std::size_t parameter_count;
};

class rhi_compute_pipeline
{
public:
    virtual ~rhi_compute_pipeline() = default;
};

struct rhi_framebuffer_desc
{
    rhi_render_pass* render_pass;
    rhi_texture* const* attachments;
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

struct rhi_buffer_barrier
{
};

struct rhi_texture_barrier
{
    rhi_texture* texture;

    rhi_access_flags src_access;
    rhi_access_flags dst_access;

    rhi_texture_layout src_layout;
    rhi_texture_layout dst_layout;
};

struct rhi_pipeline_barrier
{
    rhi_pipeline_stage_flags src_stage;
    rhi_pipeline_stage_flags dst_stage;
};

struct rhi_resource_region
{
    std::uint32_t mip_level;

    std::uint32_t layer_start;
    std::uint32_t layer_count;
};

class rhi_command
{
public:
    virtual ~rhi_command() = default;

    virtual void begin(rhi_render_pass* render_pass, rhi_framebuffer* framebuffer) = 0;
    virtual void end() = 0;
    virtual void next() = 0;

    virtual void set_render_pipeline(rhi_render_pipeline* render_pipeline) = 0;
    virtual void set_render_parameter(std::size_t index, rhi_parameter* parameter) = 0;
    virtual void set_compute_pipeline(rhi_compute_pipeline* compute_pipeline) = 0;
    virtual void set_compute_parameter(std::size_t index, rhi_parameter* parameter) = 0;

    virtual void set_viewport(const rhi_viewport& viewport) = 0;
    virtual void set_scissor(const rhi_scissor_rect* rects, std::size_t size) = 0;

    virtual void set_vertex_buffers(
        rhi_buffer* const* vertex_buffers,
        std::size_t vertex_buffer_count) = 0;
    virtual void set_index_buffer(rhi_buffer* index_buffer) = 0;

    virtual void draw(std::size_t vertex_start, std::size_t vertex_count) = 0;
    virtual void draw_indexed(
        std::size_t index_start,
        std::size_t index_count,
        std::size_t vertex_base) = 0;

    virtual void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) = 0;

    virtual void set_pipeline_barrier(
        rhi_pipeline_stage_flags src_stage,
        rhi_pipeline_stage_flags dst_stage,
        const rhi_buffer_barrier* const buffer_barriers,
        std::size_t buffer_barrier_count,
        const rhi_texture_barrier* const texture_barriers,
        std::size_t texture_barrier_count) = 0;

    virtual void copy_texture(
        rhi_texture* src,
        const rhi_resource_region& src_region,
        rhi_texture* dst,
        const rhi_resource_region& dst_region) = 0;
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

enum rhi_buffer_flag
{
    RHI_BUFFER_FLAG_VERTEX = 1 << 0,
    RHI_BUFFER_FLAG_INDEX = 1 << 1,
    RHI_BUFFER_FLAG_STORAGE = 1 << 2,
    RHI_BUFFER_FLAG_HOST_VISIBLE = 1 << 3
};
using rhi_buffer_flags = std::uint32_t;

struct rhi_buffer_desc
{
    const void* data;
    std::size_t size;

    rhi_buffer_flags flags;

    struct index_info
    {
        std::size_t size;
    } index;
};

enum rhi_texture_flag
{
    RHI_TEXTURE_FLAG_STORAGE = 1 << 0,
    RHI_TEXTURE_FLAG_MIPMAP = 1 << 1,
    RHI_TEXTURE_FLAG_RENDER_TARGET = 1 << 2,
    RHI_TEXTURE_FLAG_DEPTH_STENCIL = 1 << 3,
    RHI_TEXTURE_FLAG_TRANSFER_SRC = 1 << 4,
    RHI_TEXTURE_FLAG_TRANSFER_DST = 1 << 5
};
using rhi_texture_flags = std::uint32_t;

struct rhi_texture_desc
{
    std::uint32_t width;
    std::uint32_t height;

    rhi_format format;
    rhi_sample_count samples;

    rhi_texture_flags flags;
};

struct rhi_swapchain_desc
{
    std::uint32_t width;
    std::uint32_t height;

    void* window_handle;
};

class rhi_swapchain
{
public:
    virtual ~rhi_swapchain() = default;

    virtual rhi_semaphore* acquire_texture() = 0;

    virtual void present(
        rhi_semaphore* const* wait_semaphores,
        std::size_t wait_semaphore_count) = 0;

    virtual void resize(std::uint32_t width, std::uint32_t height) = 0;

    virtual rhi_texture* get_texture() = 0;
};

struct rhi_desc
{
    std::size_t frame_resource_count;
};

class rhi
{
public:
    virtual ~rhi() = default;

    virtual bool initialize(const rhi_desc& desc) = 0;

    virtual rhi_command* allocate_command() = 0;
    virtual void execute(
        rhi_command* const* commands,
        std::size_t command_count,
        rhi_semaphore* const* signal_semaphores,
        std::size_t signal_semaphore_count,
        rhi_semaphore* const* wait_semaphores,
        std::size_t wait_semaphore_count,
        rhi_fence* fence) = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual rhi_fence* get_in_flight_fence() = 0;

    virtual std::size_t get_frame_resource_count() const noexcept = 0;
    virtual std::size_t get_frame_resource_index() const noexcept = 0;

public:
    virtual rhi_render_pass* create_render_pass(const rhi_render_pass_desc& desc) = 0;
    virtual void destroy_render_pass(rhi_render_pass* render_pass) = 0;

    virtual rhi_shader* create_shader(const char* file) = 0;
    virtual void destroy_shader(rhi_shader* shader) = 0;

    virtual rhi_render_pipeline* create_render_pipeline(const rhi_render_pipeline_desc& desc) = 0;
    virtual void destroy_render_pipeline(rhi_render_pipeline* render_pipeline) = 0;

    virtual rhi_compute_pipeline* create_compute_pipeline(
        const rhi_compute_pipeline_desc& desc) = 0;
    virtual void destroy_compute_pipeline(rhi_compute_pipeline* compute_pipeline) = 0;

    virtual rhi_parameter* create_parameter(const rhi_parameter_desc& desc) = 0;
    virtual void destroy_parameter(rhi_parameter* parameter) = 0;

    virtual rhi_framebuffer* create_framebuffer(const rhi_framebuffer_desc& desc) = 0;
    virtual void destroy_framebuffer(rhi_framebuffer* framebuffer) = 0;

    virtual rhi_sampler* create_sampler(const rhi_sampler_desc& desc) = 0;
    virtual void destroy_sampler(rhi_sampler* sampler) = 0;

    virtual rhi_buffer* create_buffer(const rhi_buffer_desc& desc) = 0;
    virtual void destroy_buffer(rhi_buffer* buffer) = 0;

    virtual rhi_texture* create_texture(const rhi_texture_desc& desc) = 0;
    virtual rhi_texture* create_texture(const char* file, const rhi_texture_desc& desc) = 0;

    virtual rhi_texture* create_texture_cube(const rhi_texture_desc& desc) = 0;
    virtual rhi_texture* create_texture_cube(
        const char* right,
        const char* left,
        const char* top,
        const char* bottom,
        const char* front,
        const char* back,
        const rhi_texture_desc& desc) = 0;

    virtual void destroy_texture(rhi_texture* texture) = 0;

    virtual rhi_swapchain* create_swapchain(const rhi_swapchain_desc& desc) = 0;
    virtual void destroy_swapchain(rhi_swapchain* swapchain) = 0;

    virtual rhi_fence* create_fence(bool signaled) = 0;
    virtual void destroy_fence(rhi_fence* fence) = 0;

    virtual rhi_semaphore* create_semaphore() = 0;
    virtual void destroy_semaphore(rhi_semaphore* semaphore) = 0;
};
using create_rhi = rhi* (*)();
using destroy_rhi = void (*)(rhi*);
} // namespace violet
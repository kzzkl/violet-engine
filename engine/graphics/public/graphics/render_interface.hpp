#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace violet
{
struct rhi_constants
{
    static constexpr std::size_t max_attachment_count = 8;
    static constexpr std::size_t max_parameter_binding_count = 8;
    static constexpr std::size_t max_parameter_count = 16;
    static constexpr std::size_t max_vertex_attribute_count = 8;
};

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
    RHI_FORMAT_R8G8B8A8_SRGB,
    RHI_FORMAT_B8G8R8_UNORM,
    RHI_FORMAT_B8G8R8_SNORM,
    RHI_FORMAT_B8G8R8A8_UNORM,
    RHI_FORMAT_B8G8R8A8_SNORM,
    RHI_FORMAT_B8G8R8A8_UINT,
    RHI_FORMAT_B8G8R8A8_SINT,
    RHI_FORMAT_B8G8R8A8_SRGB,
    RHI_FORMAT_R16G16_UNORM,
    RHI_FORMAT_R16G16_FLOAT,
    RHI_FORMAT_R16G16B16A16_UNORM,
    RHI_FORMAT_R16G16B16A16_FLOAT,
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
    RHI_FORMAT_R11G11B10_FLOAT,
    RHI_FORMAT_D24_UNORM_S8_UINT,
    RHI_FORMAT_D32_FLOAT,
    RHI_FORMAT_D32_FLOAT_S8_UINT,
};

inline std::size_t rhi_get_format_stride(rhi_format format)
{
    switch (format)
    {
    case RHI_FORMAT_R8_UNORM:
    case RHI_FORMAT_R8_SNORM:
    case RHI_FORMAT_R8_UINT:
    case RHI_FORMAT_R8_SINT:
        return 1;
    case RHI_FORMAT_R8G8_UNORM:
    case RHI_FORMAT_R8G8_SNORM:
    case RHI_FORMAT_R8G8_UINT:
    case RHI_FORMAT_R8G8_SINT:
        return 2;
    case RHI_FORMAT_R8G8B8_UNORM:
    case RHI_FORMAT_R8G8B8_SNORM:
    case RHI_FORMAT_R8G8B8_UINT:
    case RHI_FORMAT_R8G8B8_SINT:
        return 3;
    case RHI_FORMAT_R8G8B8A8_UNORM:
    case RHI_FORMAT_R8G8B8A8_SNORM:
    case RHI_FORMAT_R8G8B8A8_UINT:
    case RHI_FORMAT_R8G8B8A8_SINT:
    case RHI_FORMAT_R8G8B8A8_SRGB:
        return 4;
    case RHI_FORMAT_B8G8R8_UNORM:
    case RHI_FORMAT_B8G8R8_SNORM:
        return 3;
    case RHI_FORMAT_B8G8R8A8_UNORM:
    case RHI_FORMAT_B8G8R8A8_SNORM:
    case RHI_FORMAT_B8G8R8A8_UINT:
    case RHI_FORMAT_B8G8R8A8_SINT:
    case RHI_FORMAT_B8G8R8A8_SRGB:
        return 4;
    case RHI_FORMAT_R16G16_UNORM:
        return 4;
    case RHI_FORMAT_R16G16B16A16_UNORM:
    case RHI_FORMAT_R16G16B16A16_FLOAT:
        return 8;
    case RHI_FORMAT_R32_UINT:
    case RHI_FORMAT_R32_SINT:
    case RHI_FORMAT_R32_FLOAT:
        return 4;
    case RHI_FORMAT_R32G32_UINT:
    case RHI_FORMAT_R32G32_SINT:
    case RHI_FORMAT_R32G32_FLOAT:
        return 8;
    case RHI_FORMAT_R32G32B32_UINT:
    case RHI_FORMAT_R32G32B32_SINT:
    case RHI_FORMAT_R32G32B32_FLOAT:
        return 12;
    case RHI_FORMAT_R32G32B32A32_UINT:
    case RHI_FORMAT_R32G32B32A32_SINT:
    case RHI_FORMAT_R32G32B32A32_FLOAT:
        return 16;
    case RHI_FORMAT_R11G11B10_FLOAT:
    case RHI_FORMAT_D24_UNORM_S8_UINT:
    case RHI_FORMAT_D32_FLOAT:
        return 4;
    case RHI_FORMAT_D32_FLOAT_S8_UINT:
        return 5;
    default:
        return 0;
    }
}

static constexpr std::uint32_t RHI_INVALID_BINDLESS_HANDLE =
    std::numeric_limits<std::uint32_t>::max();

class rhi_descriptor
{
public:
    virtual ~rhi_descriptor() = default;

    virtual std::uint32_t get_bindless() const noexcept
    {
        return RHI_INVALID_BINDLESS_HANDLE;
    }
};

enum rhi_texture_dimension
{
    RHI_TEXTURE_DIMENSION_2D,
    RHI_TEXTURE_DIMENSION_2D_ARRAY,
    RHI_TEXTURE_DIMENSION_CUBE,
};

class rhi_texture_srv : public rhi_descriptor
{
};

class rhi_texture_uav : public rhi_descriptor
{
};

class rhi_texture_rtv : public rhi_descriptor
{
};

class rhi_texture_dsv : public rhi_descriptor
{
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
    RHI_TEXTURE_LAYOUT_TRANSFER_DST,
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

enum rhi_texture_flag : std::uint32_t
{
    RHI_TEXTURE_SHADER_RESOURCE = 1 << 0,
    RHI_TEXTURE_STORAGE = 1 << 1,
    RHI_TEXTURE_RENDER_TARGET = 1 << 2,
    RHI_TEXTURE_DEPTH_STENCIL = 1 << 3,
    RHI_TEXTURE_TRANSFER_SRC = 1 << 4,
    RHI_TEXTURE_TRANSFER_DST = 1 << 5,
    RHI_TEXTURE_CUBE = 1 << 6,
};
using rhi_texture_flags = std::uint32_t;

struct rhi_texture_desc
{
    rhi_texture_extent extent;
    rhi_format format;
    rhi_texture_flags flags;

    std::uint32_t level_count{1};
    std::uint32_t layer_count{1};

    rhi_sample_count samples;
    rhi_texture_layout layout;
};

class rhi_texture
{
public:
    virtual ~rhi_texture() = default;

    virtual rhi_format get_format() const noexcept = 0;
    virtual rhi_sample_count get_samples() const noexcept = 0;
    virtual rhi_texture_extent get_extent() const noexcept = 0;

    virtual std::uint32_t get_level_count() const noexcept = 0;
    virtual std::uint32_t get_layer_count() const noexcept = 0;

    virtual rhi_texture_flags get_flags() const noexcept = 0;

    virtual rhi_texture_srv* get_srv(
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0) = 0;

    virtual rhi_texture_uav* get_uav(
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0) = 0;

    virtual rhi_texture_rtv* get_rtv(
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0) = 0;

    virtual rhi_texture_dsv* get_dsv(
        rhi_texture_dimension dimension = RHI_TEXTURE_DIMENSION_2D,
        std::uint32_t level = 0,
        std::uint32_t level_count = 0,
        std::uint32_t layer = 0,
        std::uint32_t layer_count = 0) = 0;
};

class rhi_buffer_srv : public rhi_descriptor
{
};

class rhi_buffer_uav : public rhi_descriptor
{
};

enum rhi_buffer_flag : std::uint32_t
{
    RHI_BUFFER_VERTEX = 1 << 0,
    RHI_BUFFER_INDEX = 1 << 1,
    RHI_BUFFER_UNIFORM = 1 << 2,
    RHI_BUFFER_UNIFORM_TEXEL = 1 << 3,
    RHI_BUFFER_STORAGE = 1 << 4,
    RHI_BUFFER_STORAGE_TEXEL = 1 << 5,
    RHI_BUFFER_TRANSFER_SRC = 1 << 6,
    RHI_BUFFER_TRANSFER_DST = 1 << 7,
    RHI_BUFFER_HOST_VISIBLE = 1 << 8,
    RHI_BUFFER_INDIRECT = 1 << 9,
};
using rhi_buffer_flags = std::uint32_t;

struct rhi_buffer_desc
{
    const void* data;
    std::size_t size;

    rhi_buffer_flags flags;
};

class rhi_buffer
{
public:
    virtual ~rhi_buffer() = default;

    virtual void* get_buffer_pointer() const noexcept
    {
        return nullptr;
    }

    virtual std::size_t get_buffer_size() const noexcept = 0;

    virtual rhi_buffer_srv* get_srv(
        std::size_t offset = 0,
        std::size_t size = 0,
        rhi_format format = RHI_FORMAT_UNDEFINED) = 0;
    virtual rhi_buffer_uav* get_uav(
        std::size_t offset = 0,
        std::size_t size = 0,
        rhi_format format = RHI_FORMAT_UNDEFINED) = 0;
};

enum rhi_filter
{
    RHI_FILTER_POINT,
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

    float min_level;
    float max_level;
};

class rhi_sampler : public rhi_descriptor
{
public:
    virtual ~rhi_sampler() = default;
};

enum rhi_attachment_type
{
    RHI_ATTACHMENT_RENDER_TARGET,
    RHI_ATTACHMENT_DEPTH_STENCIL,
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
    rhi_attachment_type type;

    rhi_format format;
    rhi_sample_count samples;

    rhi_texture_layout layout;
    rhi_texture_layout initial_layout;
    rhi_texture_layout final_layout;

    rhi_attachment_load_op load_op;
    rhi_attachment_store_op store_op;
    rhi_attachment_load_op stencil_load_op;
    rhi_attachment_store_op stencil_store_op;
};

enum rhi_pipeline_stage_flag : std::uint32_t
{
    RHI_PIPELINE_STAGE_BEGIN = 1 << 0,
    RHI_PIPELINE_STAGE_VERTEX_INPUT = 1 << 1,
    RHI_PIPELINE_STAGE_VERTEX = 1 << 2,
    RHI_PIPELINE_STAGE_EARLY_DEPTH_STENCIL = 1 << 3,
    RHI_PIPELINE_STAGE_FRAGMENT = 1 << 4,
    RHI_PIPELINE_STAGE_LATE_DEPTH_STENCIL = 1 << 5,
    RHI_PIPELINE_STAGE_COLOR_OUTPUT = 1 << 6,
    RHI_PIPELINE_STAGE_TRANSFER = 1 << 7,
    RHI_PIPELINE_STAGE_COMPUTE = 1 << 8,
    RHI_PIPELINE_STAGE_END = 1 << 9,
    RHI_PIPELINE_STAGE_HOST = 1 << 10,
    RHI_PIPELINE_STAGE_DRAW_INDIRECT = 1 << 11,
};
using rhi_pipeline_stage_flags = std::uint32_t;

enum rhi_access_flag : std::uint32_t
{
    RHI_ACCESS_COLOR_READ = 1 << 0,
    RHI_ACCESS_COLOR_WRITE = 1 << 1,
    RHI_ACCESS_DEPTH_STENCIL_READ = 1 << 2,
    RHI_ACCESS_DEPTH_STENCIL_WRITE = 1 << 3,
    RHI_ACCESS_SHADER_READ = 1 << 4,
    RHI_ACCESS_SHADER_WRITE = 1 << 5,
    RHI_ACCESS_TRANSFER_READ = 1 << 6,
    RHI_ACCESS_TRANSFER_WRITE = 1 << 7,
    RHI_ACCESS_HOST_READ = 1 << 8,
    RHI_ACCESS_HOST_WRITE = 1 << 9,
    RHI_ACCESS_INDIRECT_COMMAND_READ = 1 << 10,
    RHI_ACCESS_VERTEX_ATTRIBUTE_READ = 1 << 11,
};
using rhi_access_flags = std::uint32_t;

static constexpr std::size_t RHI_RENDER_SUBPASS_EXTERNAL = ~0U;

struct rhi_render_pass_dependency_desc
{
    rhi_pipeline_stage_flags src_stages;
    rhi_access_flags src_access;

    rhi_pipeline_stage_flags dst_stages;
    rhi_access_flags dst_access;
};

struct rhi_render_pass_desc
{
    rhi_attachment_desc attachments[rhi_constants::max_attachment_count];
    std::size_t attachment_count;

    rhi_render_pass_dependency_desc begin_dependency;
    rhi_render_pass_dependency_desc end_dependency;
};

class rhi_render_pass
{
public:
    virtual ~rhi_render_pass() = default;
};

enum rhi_shader_stage_flag : std::uint32_t
{
    RHI_SHADER_STAGE_VERTEX = 1 << 0,
    RHI_SHADER_STAGE_FRAGMENT = 1 << 1,
    RHI_SHADER_STAGE_COMPUTE = 1 << 2,
    RHI_SHADER_STAGE_ALL = 0x7FFFFFFF
};
using rhi_shader_stage_flags = std::uint32_t;

enum rhi_parameter_binding_type
{
    RHI_PARAMETER_BINDING_CONSTANT,
    RHI_PARAMETER_BINDING_UNIFORM_BUFFER,
    RHI_PARAMETER_BINDING_UNIFORM_TEXEL,
    RHI_PARAMETER_BINDING_STORAGE_BUFFER,
    RHI_PARAMETER_BINDING_STORAGE_TEXEL,
    RHI_PARAMETER_BINDING_STORAGE_TEXTURE,
    RHI_PARAMETER_BINDING_TEXTURE,
    RHI_PARAMETER_BINDING_SAMPLER,
    RHI_PARAMETER_BINDING_MUTABLE,
};

struct rhi_parameter_binding
{
    rhi_parameter_binding_type type;
    rhi_shader_stage_flags stages;
    std::size_t size;
};

enum rhi_parameter_flag : std::uint32_t
{
    RHI_PARAMETER_SIMPLE = 1 << 0,
    RHI_PARAMETER_DISABLE_SYNC = 1 << 1,
};
using rhi_parameter_flags = std::uint32_t;

struct rhi_parameter_desc
{
    const rhi_parameter_binding* bindings;
    std::size_t binding_count;

    rhi_parameter_flags flags;
};

class rhi_parameter
{
public:
    virtual ~rhi_parameter() = default;

    virtual void set_constant(
        std::size_t index,
        const void* data,
        std::size_t size,
        std::size_t offset = 0) = 0;
    virtual void set_srv(std::size_t index, rhi_texture_srv* srv, std::size_t offset = 0) = 0;
    virtual void set_srv(std::size_t index, rhi_buffer_srv* srv, std::size_t offset = 0) = 0;
    virtual void set_uav(std::size_t index, rhi_texture_uav* uav, std::size_t offset = 0) = 0;
    virtual void set_uav(std::size_t index, rhi_buffer_uav* uav, std::size_t offset = 0) = 0;
    virtual void set_sampler(std::size_t index, rhi_sampler* sampler, std::size_t offset = 0) = 0;
};

enum rhi_primitive_topology
{
    RHI_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    RHI_PRIMITIVE_TOPOLOGY_LINE_LIST
};

struct rhi_vertex_attribute
{
    const char* name;
    rhi_format format;
};

struct rhi_shader_desc
{
    std::uint8_t* code;
    std::size_t code_size;

    rhi_shader_stage_flag stage;

    struct parameter_slot
    {
        std::uint32_t space;
        rhi_parameter_desc desc;
    };

    const parameter_slot* parameters;
    std::size_t parameter_count;

    struct
    {
        const rhi_vertex_attribute* attributes;
        std::size_t attribute_count;
    } vertex;
};

class rhi_shader
{
public:
    virtual ~rhi_shader() = default;
};

enum rhi_blend_factor
{
    RHI_BLEND_FACTOR_ZERO,
    RHI_BLEND_FACTOR_ONE,
    RHI_BLEND_FACTOR_SRC_COLOR,
    RHI_BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
    RHI_BLEND_FACTOR_SRC_ALPHA,
    RHI_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
    RHI_BLEND_FACTOR_DST_COLOR,
    RHI_BLEND_FACTOR_ONE_MINUS_DST_COLOR,
    RHI_BLEND_FACTOR_DST_ALPHA,
    RHI_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
};

enum rhi_blend_op
{
    RHI_BLEND_OP_ADD,
    RHI_BLEND_OP_SUBTRACT,
    RHI_BLEND_OP_MIN,
    RHI_BLEND_OP_MAX,
    RHI_BLEND_OP_MULTIPLY,
};

struct rhi_attachment_blend
{
    bool enable;

    rhi_blend_factor src_color_factor;
    rhi_blend_factor dst_color_factor;
    rhi_blend_op color_op;

    rhi_blend_factor src_alpha_factor;
    rhi_blend_factor dst_alpha_factor;
    rhi_blend_op alpha_op;
};

struct rhi_blend_state
{
    rhi_attachment_blend attachments[rhi_constants::max_attachment_count];
};

enum rhi_compare_op
{
    RHI_COMPARE_OP_NEVER,
    RHI_COMPARE_OP_LESS,
    RHI_COMPARE_OP_EQUAL,
    RHI_COMPARE_OP_LESS_EQUAL,
    RHI_COMPARE_OP_GREATER,
    RHI_COMPARE_OP_NOT_EQUAL,
    RHI_COMPARE_OP_GREATER_EQUAL,
    RHI_COMPARE_OP_ALWAYS,
};

enum rhi_stencil_op
{
    RHI_STENCIL_OP_KEEP,
    RHI_STENCIL_OP_ZERO,
    RHI_STENCIL_OP_REPLACE,
    RHI_STENCIL_OP_INCR,
    RHI_STENCIL_OP_DECR,
    RHI_STENCIL_OP_INCR_CLAMP,
    RHI_STENCIL_OP_DECR_CLAMP,
    RHI_STENCIL_OP_INVERT,
};

struct rhi_stencil_state
{
    rhi_compare_op compare_op;
    rhi_stencil_op pass_op;
    rhi_stencil_op fail_op;
    rhi_stencil_op depth_fail_op;

    std::uint32_t reference;
};

struct rhi_depth_stencil_state
{
    bool depth_enable;
    bool depth_write_enable;
    rhi_compare_op depth_compare_op;

    bool stencil_enable;
    rhi_stencil_state stencil_front;
    rhi_stencil_state stencil_back;
};

enum rhi_cull_mode
{
    RHI_CULL_MODE_NONE,
    RHI_CULL_MODE_FRONT,
    RHI_CULL_MODE_BACK
};

struct rhi_rasterizer_state
{
    rhi_cull_mode cull_mode;
};

struct rhi_raster_pipeline_desc
{
    rhi_shader* vertex_shader;
    rhi_shader* fragment_shader;

    rhi_blend_state blend;
    rhi_depth_stencil_state depth_stencil;
    rhi_rasterizer_state rasterizer;

    rhi_sample_count samples;
    rhi_primitive_topology primitive_topology;

    rhi_render_pass* render_pass;
};

class rhi_raster_pipeline
{
public:
    virtual ~rhi_raster_pipeline() = default;
};

struct rhi_compute_pipeline_desc
{
    rhi_shader* compute_shader;
};

class rhi_compute_pipeline
{
public:
    virtual ~rhi_compute_pipeline() = default;
};

union rhi_clear_value
{
    float color[4];

    struct
    {
        float depth;
        std::uint32_t stencil;
    };
};

struct rhi_attachment
{
    union
    {
        rhi_texture_rtv* rtv;
        rhi_texture_dsv* dsv;
    };

    rhi_clear_value clear_value;
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
    rhi_buffer* buffer;

    rhi_pipeline_stage_flags src_stages;
    rhi_access_flags src_access;

    rhi_pipeline_stage_flags dst_stages;
    rhi_access_flags dst_access;

    std::size_t offset;
    std::size_t size;
};

struct rhi_texture_barrier
{
    rhi_texture* texture;

    rhi_pipeline_stage_flags src_stages;
    rhi_access_flags src_access;
    rhi_texture_layout src_layout;

    rhi_pipeline_stage_flags dst_stages;
    rhi_access_flags dst_access;
    rhi_texture_layout dst_layout;

    std::uint32_t level;
    std::uint32_t level_count;
    std::uint32_t layer;
    std::uint32_t layer_count;
};

struct rhi_texture_region
{
    std::int32_t offset_x;
    std::int32_t offset_y;
    rhi_texture_extent extent;

    std::uint32_t level;
    std::uint32_t layer;
    std::uint32_t layer_count;
};

struct rhi_buffer_region
{
    std::size_t offset;
    std::size_t size;
};

class rhi_fence
{
public:
    virtual ~rhi_fence() = default;

    virtual void wait(std::uint64_t value) = 0;
};

class rhi_command
{
public:
    virtual ~rhi_command() = default;

    virtual void begin_render_pass(
        rhi_render_pass* render_pass,
        const rhi_attachment* attachments,
        std::size_t attachment_count) = 0;
    virtual void end_render_pass() = 0;

    virtual void set_pipeline(rhi_raster_pipeline* raster_pipeline) = 0;
    virtual void set_pipeline(rhi_compute_pipeline* compute_pipeline) = 0;
    virtual void set_parameter(std::size_t index, rhi_parameter* parameter) = 0;

    virtual void set_viewport(const rhi_viewport& viewport) = 0;
    virtual void set_scissor(const rhi_scissor_rect* rects, std::size_t size) = 0;

    virtual void set_vertex_buffers(
        rhi_buffer* const* vertex_buffers,
        std::size_t vertex_buffer_count) = 0;
    virtual void set_index_buffer(rhi_buffer* index_buffer, std::size_t index_size) = 0;

    virtual void draw(
        std::uint32_t vertex_offset,
        std::uint32_t vertex_count,
        std::uint32_t instance_offset = 0,
        std::uint32_t instance_count = 1) = 0;
    virtual void draw_indexed(
        std::uint32_t index_offset,
        std::uint32_t index_count,
        std::uint32_t vertex_offset,
        std::uint32_t instance_offset = 0,
        std::uint32_t instance_count = 1) = 0;
    virtual void draw_indexed_indirect(
        rhi_buffer* command_buffer,
        std::size_t command_buffer_offset,
        rhi_buffer* count_buffer,
        std::size_t count_buffer_offset,
        std::size_t max_draw_count) = 0;

    virtual void dispatch(std::uint32_t x, std::uint32_t y, std::uint32_t z) = 0;

    virtual void set_pipeline_barrier(
        const rhi_buffer_barrier* buffer_barriers,
        std::size_t buffer_barrier_count,
        const rhi_texture_barrier* texture_barriers,
        std::size_t texture_barrier_count) = 0;

    virtual void copy_texture(
        rhi_texture* src,
        const rhi_texture_region& src_region,
        rhi_texture* dst,
        const rhi_texture_region& dst_region) = 0;

    virtual void blit_texture(
        rhi_texture* src,
        const rhi_texture_region& src_region,
        rhi_texture* dst,
        const rhi_texture_region& dst_region) = 0;

    virtual void fill_buffer(
        rhi_buffer* buffer,
        const rhi_buffer_region& region,
        std::uint32_t value) = 0;

    virtual void copy_buffer(
        rhi_buffer* src,
        const rhi_buffer_region& src_region,
        rhi_buffer* dst,
        const rhi_buffer_region& dst_region) = 0;

    virtual void copy_buffer_to_texture(
        rhi_buffer* buffer,
        const rhi_buffer_region& buffer_region,
        rhi_texture* texture,
        const rhi_texture_region& texture_region) = 0;

    virtual void signal(rhi_fence* fence, std::uint64_t value) = 0;
    virtual void wait(
        rhi_fence* fence,
        std::uint64_t value,
        rhi_pipeline_stage_flags stages = RHI_PIPELINE_STAGE_BEGIN) = 0;

    virtual void begin_label(const char* label) const = 0;
    virtual void end_label() const = 0;
};

struct rhi_swapchain_desc
{
    rhi_texture_flags flags;
    void* window_handle;
};

class rhi_swapchain
{
public:
    virtual ~rhi_swapchain() = default;

    virtual rhi_fence* acquire_texture() = 0;
    virtual rhi_fence* get_present_fence() const = 0;

    virtual void present() = 0;

    /**
     * @brief Mark the swapchain out of date and will be resized.
     */
    virtual void resize() = 0;

    virtual rhi_texture* get_texture() = 0;
    virtual rhi_texture* get_texture(std::size_t index) = 0;
    virtual std::size_t get_texture_count() const noexcept = 0;
};

enum rhi_backend
{
    RHI_BACKEND_VULKAN
};

enum rhi_feature : std::uint32_t
{
    RHI_FEATURE_INDIRECT_DRAW = 1 << 0,
    RHI_FEATURE_BINDLESS = 1 << 1,
};
using rhi_features = std::uint32_t;

struct rhi_desc
{
    rhi_features features;
    std::size_t frame_resource_count;
};

class rhi
{
public:
    virtual ~rhi() = default;

    virtual bool initialize(const rhi_desc& desc) = 0;

    virtual rhi_command* allocate_command() = 0;
    virtual void execute(rhi_command* command) = 0;

    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual std::size_t get_frame_count() const noexcept = 0;
    virtual std::size_t get_frame_resource_count() const noexcept = 0;
    virtual std::size_t get_frame_resource_index() const noexcept = 0;

    virtual rhi_parameter* get_bindless_parameter() const noexcept = 0;

    virtual rhi_backend get_backend() const noexcept = 0;

    virtual void set_name(rhi_texture* object, const char* name) const = 0;
    virtual void set_name(rhi_buffer* object, const char* name) const = 0;

    virtual rhi_render_pass* create_render_pass(const rhi_render_pass_desc& desc) = 0;
    virtual void destroy_render_pass(rhi_render_pass* render_pass) = 0;

    virtual rhi_shader* create_shader(const rhi_shader_desc& desc) = 0;
    virtual void destroy_shader(rhi_shader* shader) = 0;

    virtual rhi_raster_pipeline* create_raster_pipeline(const rhi_raster_pipeline_desc& desc) = 0;
    virtual void destroy_raster_pipeline(rhi_raster_pipeline* raster_pipeline) = 0;

    virtual rhi_compute_pipeline* create_compute_pipeline(
        const rhi_compute_pipeline_desc& desc) = 0;
    virtual void destroy_compute_pipeline(rhi_compute_pipeline* compute_pipeline) = 0;

    virtual rhi_parameter* create_parameter(
        const rhi_parameter_desc& desc,
        bool auto_sync = true) = 0;
    virtual void destroy_parameter(rhi_parameter* parameter) = 0;

    virtual rhi_sampler* create_sampler(const rhi_sampler_desc& desc) = 0;
    virtual void destroy_sampler(rhi_sampler* sampler) = 0;

    virtual rhi_buffer* create_buffer(const rhi_buffer_desc& desc) = 0;
    virtual void destroy_buffer(rhi_buffer* buffer) = 0;

    virtual rhi_texture* create_texture(const rhi_texture_desc& desc) = 0;
    virtual void destroy_texture(rhi_texture* texture) = 0;

    virtual rhi_swapchain* create_swapchain(const rhi_swapchain_desc& desc) = 0;
    virtual void destroy_swapchain(rhi_swapchain* swapchain) = 0;

    virtual rhi_fence* create_fence() = 0;
    virtual void destroy_fence(rhi_fence* fence) = 0;
};
using create_rhi = rhi* (*)();
using destroy_rhi = void (*)(rhi*);
} // namespace violet
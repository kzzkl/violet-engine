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
    R32G32B32A32_FLOAT,
    D32_FLOAT_S8_UINT
};

enum class resource_state
{
    UNDEFINED,
    PRESENT,
    DEPTH_STENCIL
};

class resource_interface
{
};
using resource = resource_interface;

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

struct vertex_layout_desc
{
    vertex_attribute_type* attributes;
    std::size_t attribute_count;
};

enum class pass_parameter_type : std::uint8_t
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

struct pass_parameter_pair
{
    pass_parameter_type type;
    std::size_t size; // Only for array.
};

struct pass_parameter_layout_desc
{
    pass_parameter_pair* parameters;
    std::size_t size;
};

class pass_parameter_layout_interface
{
};

struct pass_layout_desc
{
    pass_parameter_layout_interface** parameters;
    std::size_t size;
};

class pass_layout_interface
{
};

class pass_parameter_interface
{
public:
    virtual ~pass_parameter_interface() = default;

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

struct pass_blend_desc
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

struct pass_depth_stencil_desc
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

struct pass_desc
{
    const char* vertex_shader;
    const char* pixel_shader;

    vertex_layout_desc vertex_layout;
    pass_layout_interface* pass_layout;

    pass_blend_desc blend;
    pass_depth_stencil_desc depth_stencil;

    std::size_t* input;
    std::size_t input_count;

    std::size_t* output;
    std::size_t output_count;

    std::size_t depth;
    bool output_depth;

    primitive_topology_type primitive_topology;
};

struct render_target_desc
{
    enum class load_op_type
    {
        LOAD,
        CLEAR,
        DONT_CARE
    };

    enum class store_op_type
    {
        STORE,
        DONT_CARE
    };

    resource_format format;

    load_op_type load_op;
    store_op_type store_op;
    load_op_type stencil_load_op;
    store_op_type stencil_store_op;

    resource_state initial_state;
    resource_state final_state;

    std::size_t samples;
};

struct technique_desc
{
    render_target_desc* render_targets;
    std::size_t render_target_count;

    pass_desc* subpasses;
    std::size_t subpass_count;
};

class technique_interface
{
};

struct render_target_set_desc
{
    resource_interface** render_targets;
    std::size_t render_target_count;

    std::uint32_t width;
    std::uint32_t height;

    technique_interface* technique;
};

class render_target_set_interface
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
        technique_interface* pass,
        render_target_set_interface* render_target_set) = 0;
    virtual void end(technique_interface* pass) = 0;
    virtual void next(technique_interface* pass) = 0;

    virtual void parameter(std::size_t i, pass_parameter_interface*) = 0;

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

    std::size_t multiple_sampling;
    std::size_t frame_resource;
    std::size_t render_concurrency;
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

class factory_interface
{
public:
    virtual renderer_interface* make_renderer(const renderer_desc& desc) = 0;

    virtual render_target_set_interface* make_render_target_set(
        const render_target_set_desc& desc) = 0;
    virtual technique_interface* make_technique(const technique_desc& desc) = 0;

    virtual pass_parameter_layout_interface* make_pass_parameter_layout(
        const pass_parameter_layout_desc& desc) = 0;
    virtual pass_parameter_interface* make_pass_parameter(
        pass_parameter_layout_interface* layout) = 0;
    virtual pass_layout_interface* make_pass_layout(const pass_layout_desc& desc) = 0;

    virtual resource_interface* make_vertex_buffer(const vertex_buffer_desc& desc) = 0;
    virtual resource_interface* make_index_buffer(const index_buffer_desc& desc) = 0;

    virtual resource_interface* make_texture(
        const std::uint8_t* data,
        std::uint32_t width,
        std::uint32_t height) = 0;
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
using factory = factory_interface;

using make_factory = factory_interface* (*)();
} // namespace ash::graphics
#pragma once

#include "plugin_interface.hpp"
#include <cstddef>
#include <cstdint>

namespace ash::graphics::vk
{
class resource
{
};

class render_parameter
{
public:
    void set(std::size_t i) {}

private:
};

struct vertex_layout
{
};

struct parameter_layout
{
};

struct render_pipeline_blend_desc
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

struct render_pipeline_depth_stencil_desc
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

struct render_pipeline_desc
{
    const char* vertex_shader;
    const char* pixel_shader;

    vertex_layout* vertex_layout;
    parameter_layout* parameter_layout;

    render_pipeline_blend_desc blend;
    render_pipeline_depth_stencil_desc depth_stencil;

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
    render_pipeline_desc* subpasses;
    std::size_t subpass_count;
};

class render_pipeline
{
};

class render_pass
{
public:
    virtual render_pipeline* subpass(std::size_t index) = 0;
};

struct frame_buffer_desc
{
    resource** resources;
    std::size_t resource_count;

    std::uint32_t width;
    std::uint32_t height;

    render_pass* render_pass;
};

class frame_buffer
{
};

class render_command
{
public:
    virtual void begin(render_pass* pass, frame_buffer* frame_buffer) = 0;
    virtual void end(render_pass* pass) = 0;

    virtual void begin(render_pipeline* pass) = 0;
    virtual void end(render_pipeline* pass) = 0;

    virtual void parameter(std::size_t i, render_parameter*) = 0;

    virtual void draw(
        resource* vertex,
        resource* index,
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

class renderer
{
public:
    virtual std::size_t begin_frame() = 0;
    virtual void end_frame() = 0;

    virtual render_command* allocate_command() = 0;
    virtual void execute(render_command*) = 0;

    virtual resource* back_buffer(std::size_t index) = 0;
    virtual std::size_t back_buffer_count() = 0;

    virtual resource* depth_stencil() = 0;
};

class factory
{
public:
    virtual frame_buffer* make_frame_buffer(const frame_buffer_desc& desc) = 0;
    virtual render_pass* make_render_pass(const render_pass_desc& desc) = 0;
    virtual renderer* make_renderer(const renderer_desc& desc) = 0;
};

using make_factory = factory* (*)();
} // namespace ash::graphics::vk
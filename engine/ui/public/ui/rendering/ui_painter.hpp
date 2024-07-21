#pragma once

#include "graphics/render_device.hpp"
#include "math/math.hpp"
#include "ui/color.hpp"
#include "ui/font.hpp"
#include <stack>
#include <unordered_map>
#include <vector>

namespace violet
{
struct ui_draw_mesh
{
    std::size_t vertex_start;
    std::size_t index_start;
    std::size_t index_count;

    rhi_parameter* parameter;
};

struct ui_draw_batch
{
    std::vector<float2> position;
    std::vector<float2> uv;
    std::vector<std::uint32_t> color;
    std::vector<std::uint32_t> index;
};

struct ui_draw_group
{
    bool scissor;
    rhi_scissor_rect scissor_rect;
    std::unordered_map<rhi_texture*, ui_draw_batch*> batches;

    std::vector<ui_draw_mesh> meshes;
};

class ui_painter
{
public:
    ui_painter(font* default_font, render_device* device);

    void push_group(bool scissor = false, const rhi_scissor_rect& scissor_rect = {});
    void pop_group();

    void set_color(ui_color color) noexcept { m_pen_color = color; }
    void set_position(float x, float y) noexcept
    {
        m_pen_position[0] = x;
        m_pen_position[1] = y;
    }

    void draw_rect(float width, float height);
    void draw_text(std::string_view text, font* font = nullptr);

    void draw_line(const float2& start, const float2& end, float thickness);
    void draw_path(std::span<const float2> points, float thickness);

    void draw_mesh(
        std::span<const float2> position,
        std::span<const std::uint32_t> color,
        std::span<const float2> uv,
        std::span<const std::uint32_t> indices,
        rhi_texture* texture = nullptr);

    void set_extent(std::uint32_t width, std::uint32_t height);

    std::uint32_t get_text_width(std::string_view text, font* font = nullptr);
    std::uint32_t get_text_height(std::string_view text, font* font = nullptr);
    font* get_default_font() const noexcept { return m_default_font; }

    rhi_parameter* get_mvp_parameter() const noexcept { return m_mvp_parameter.get(); }

    std::vector<rhi_buffer*> get_vertex_buffers() const
    {
        return {m_position_buffer.get(), m_color_buffer.get(), m_uv_buffer.get()};
    }

    rhi_buffer* get_index_buffer() const noexcept { return m_index_buffer.get(); }

    template <typename Functor>
    void each_group(Functor functor)
    {
        for (std::size_t i = 0; i < m_group_count; ++i)
            functor(m_group_pool[i].get());
    }

    void compile();
    void reset();

private:
    ui_draw_group* allocate_group();
    ui_draw_batch* allocate_batch();
    rhi_parameter* allocate_parameter();

    ui_color m_pen_color{ui_color::WHITE};
    float2 m_pen_position{};

    font* m_default_font;

    std::stack<ui_draw_group*> m_group_stack;
    std::size_t m_group_count{0};
    std::vector<std::unique_ptr<ui_draw_group>> m_group_pool;

    rhi_ptr<rhi_buffer> m_position_buffer;
    rhi_ptr<rhi_buffer> m_color_buffer;
    rhi_ptr<rhi_buffer> m_uv_buffer;
    rhi_ptr<rhi_buffer> m_index_buffer;

    std::size_t m_batch_count{0};
    std::vector<std::unique_ptr<ui_draw_batch>> m_batche_pool;

    rhi_ptr<rhi_parameter> m_mvp_parameter;
    rhi_ptr<rhi_sampler> m_sampler;

    std::size_t m_parameter_count{0};
    std::vector<rhi_ptr<rhi_parameter>> m_parameter_pool;

    render_device* m_device;
};

namespace pipeline_parameter
{
static constexpr shader_parameter ui_mvp = {
    {RHI_PARAMETER_TYPE_UNIFORM, sizeof(float4x4), RHI_SHADER_STAGE_VERTEX}
};
static constexpr shader_parameter ui_material = {
    {RHI_PARAMETER_TYPE_UNIFORM, sizeof(std::uint32_t), RHI_SHADER_STAGE_FRAGMENT},
    {RHI_PARAMETER_TYPE_TEXTURE, 1,                     RHI_SHADER_STAGE_FRAGMENT}
};
} // namespace pipeline_parameter
} // namespace violet
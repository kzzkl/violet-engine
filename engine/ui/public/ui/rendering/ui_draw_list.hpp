#pragma once

#include "graphics/render_device.hpp"
#include "math/math.hpp"
#include <memory>
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

    rhi_texture* texture;
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
    rhi_scissor_rect scissor;
    std::unordered_map<rhi_texture*, ui_draw_batch*> batches;
};

class ui_draw_list
{
public:
    ui_draw_list(render_device* device);

    void push_scissor(const rhi_scissor_rect& scissor);
    void pop_scissor();

    void draw_box(float x, float y, float width, float height);

    void draw(
        const std::vector<float2>& position,
        const std::vector<float2>& uv,
        const std::vector<std::uint32_t>& color,
        const std::vector<std::uint32_t>& indices,
        rhi_texture* texture = nullptr);

    std::vector<rhi_buffer*> get_vertex_buffers() const
    {
        return {m_position.get(), m_uv.get(), m_color.get()};
    }

    rhi_buffer* get_index_buffer() const noexcept { return m_index.get(); }

    const std::vector<ui_draw_mesh> get_meshes() const noexcept { return m_meshes; }

    void compile();
    void reset();

private:
    ui_draw_batch* allocate_batch();

    std::stack<rhi_scissor_rect> m_scissor_stack;
    std::vector<ui_draw_group> m_groups;

    std::size_t m_used_batch_count;
    std::vector<std::unique_ptr<ui_draw_batch>> m_batches;

    rhi_ptr<rhi_buffer> m_position;
    rhi_ptr<rhi_buffer> m_uv;
    rhi_ptr<rhi_buffer> m_color;
    rhi_ptr<rhi_buffer> m_index;

    std::vector<ui_draw_mesh> m_meshes;

    render_device* m_device;
};
} // namespace violet
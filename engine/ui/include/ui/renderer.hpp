#pragma once

#include "element.hpp"
#include <memory>
#include <stack>
#include <unordered_map>

namespace ash::ui
{
struct render_batch
{
    element_mesh_type type;
    graphics::resource* texture;

    element_extent scissor;

    std::vector<math::float2> vertex_position;
    std::vector<math::float2> vertex_uv;
    std::vector<std::uint32_t> vertex_color;
    std::vector<std::uint32_t> vertex_offset_index;
    std::vector<std::uint32_t> indices;
};

class renderer
{
public:
    renderer();

    void draw(element* root);
    void reset();

    auto begin() { return m_batch_pool.begin(); }
    auto end() { return m_batch_pool.begin() + m_batch_pool_index; }

    const std::vector<math::float4>& offset() const noexcept { return m_offset; }

private:
    struct batch_map
    {
        element_extent scissor;

        render_batch* block_batch;
        std::unordered_map<graphics::resource*, render_batch*> text_batch;
        std::unordered_map<graphics::resource*, render_batch*> image_batch;
    };

    void draw(batch_map& batch_map, const element_mesh& mesh, float x, float y, float depth);

    render_batch* allocate_batch(
        element_mesh_type type,
        const element_extent& scissor,
        graphics::resource* texture);

    std::vector<math::float4> m_offset; // x, y, depth

    std::size_t m_batch_pool_index;
    std::vector<std::unique_ptr<render_batch>> m_batch_pool;
};
} // namespace ash::ui
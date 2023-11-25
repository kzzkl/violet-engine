#pragma once

#include "ecs/entity.hpp"
#include "graphics/mesh_render.hpp"
#include "render/ui_pipeline.hpp"
#include "ui/control.hpp"
#include <memory>
#include <stack>
#include <unordered_map>

namespace violet::ui
{
struct render_batch
{
    control_mesh_type type;
    graphics::rhi_resource* texture;

    node_rect scissor;

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

    void draw(control* root);
    void reset();

    void resize(std::uint32_t width, std::uint32_t height);

private:
    struct batch_map
    {
        node_rect scissor;

        render_batch* block_batch;
        std::unordered_map<graphics::rhi_resource*, render_batch*> text_batch;
        std::unordered_map<graphics::rhi_resource*, render_batch*> image_batch;
    };

    void draw(batch_map& batch_map, const control_mesh& mesh, float x, float y, float depth);
    void begin_draw(graphics::mesh_render& mesh);
    void end_draw(graphics::mesh_render& mesh);

    render_batch* allocate_batch(
        control_mesh_type type,
        const node_rect& scissor,
        graphics::rhi_resource* texture);

    material_pipeline_parameter* allocate_material_parameter();

    ecs::entity m_entity;

    std::vector<math::float4> m_offset; // x, y, depth

    std::size_t m_batch_pool_index;
    std::vector<std::unique_ptr<render_batch>> m_batch_pool;

    std::unique_ptr<ui_pipeline> m_pipeline;

    std::vector<std::unique_ptr<graphics::rhi_resource>> m_vertex_buffers;
    std::unique_ptr<graphics::rhi_resource> m_index_buffer;

    std::size_t m_material_parameter_counter;
    std::vector<std::unique_ptr<material_pipeline_parameter>> m_material_parameter_pool;
};
} // namespace violet::ui
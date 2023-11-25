#include "render/renderer.hpp"
#include "common/log.hpp"
#include "ecs/world.hpp"
#include <algorithm>

namespace violet::ui
{
static constexpr std::size_t MAX_UI_VERTEX_COUNT = 4096 * 16;
static constexpr std::size_t MAX_UI_INDEX_COUNT = MAX_UI_VERTEX_COUNT * 2;

renderer::renderer() : m_batch_pool_index(0), m_material_parameter_counter(0)
{
    auto& world = system<ecs::world>();
    m_entity = world.create("ui root");
    world.add<graphics::mesh_render>(m_entity);

    // Position.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<math::float2>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // UV.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<math::float2>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // Color.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<std::uint32_t>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    // Offset index.
    m_vertex_buffers.push_back(graphics::rhi::make_vertex_buffer<std::uint32_t>(
        nullptr,
        MAX_UI_VERTEX_COUNT,
        graphics::VERTEX_BUFFER_FLAG_NONE,
        true,
        true));
    m_index_buffer =
        graphics::rhi::make_index_buffer<std::uint32_t>(nullptr, MAX_UI_INDEX_COUNT, true, true);

    auto& mesh_render = world.component<graphics::mesh_render>(m_entity);
    mesh_render.render_groups = graphics::RENDER_GROUP_UI;
    for (auto& vertex_buffer : m_vertex_buffers)
        mesh_render.vertex_buffers.push_back(vertex_buffer.get());
    mesh_render.index_buffer = m_index_buffer.get();

    m_pipeline = std::make_unique<ui_pipeline>();
}

void renderer::draw(control* root)
{
    auto& world = system<ecs::world>();
    auto& target = world.component<graphics::mesh_render>(m_entity);

    begin_draw(target);

    std::stack<std::pair<control*, std::size_t>> dfs;
    std::stack<batch_map> scissor_stack;
    std::stack<node_rect> visible_area_stack;

    auto calculate_overlap_area = [](const node_rect& a, const node_rect& b) {
        float x1 = std::max(a.x, b.x);
        float x2 = std::min(a.x + a.width, b.x + b.width);
        float y1 = std::max(a.y, b.y);
        float y2 = std::min(a.y + a.height, b.y + b.height);
        return node_rect{.x = x1, .y = y1, .width = x2 - x1, .height = y2 - y1};
    };

    dfs.push({root, 0});
    visible_area_stack.push(root->extent());

    while (!dfs.empty())
    {
        auto [node, batch_map_index] = dfs.top();
        dfs.pop();

        if (batch_map_index != scissor_stack.size())
            scissor_stack.pop();

        auto& extent = node->extent();

        auto mesh = node->mesh();
        if (mesh != nullptr)
        {
            if (mesh->scissor)
            {
                scissor_stack.push(batch_map{.scissor = extent});
                batch_map_index = scissor_stack.size();
            }

            draw(scissor_stack.top(), *mesh, extent.x, extent.y, node->depth());
        }

        node_rect visible_area = visible_area_stack.top();
        visible_area_stack.pop();
        for (control* child : node->children())
        {
            if (!child->display())
                continue;

            node_rect overlap_area = calculate_overlap_area(visible_area, child->extent());
            if (overlap_area.height <= 0 || overlap_area.width <= 0)
                continue;
            else
                visible_area_stack.push(overlap_area);

            dfs.push({child, batch_map_index});
        }
    }

    end_draw(target);
}

void renderer::reset()
{
    for (auto& batch : m_batch_pool)
    {
        batch->vertex_position.clear();
        batch->vertex_uv.clear();
        batch->vertex_color.clear();
        batch->vertex_offset_index.clear();
        batch->indices.clear();
    }
    m_batch_pool_index = 0;
    m_offset.clear();
}

void renderer::resize(std::uint32_t width, std::uint32_t height)
{
    math::float4x4 orthographic = math::matrix::orthographic(
        0.0f,
        static_cast<float>(width),
        static_cast<float>(height),
        0.0f,
        0.0f,
        1.0f);
    m_pipeline->set_mvp_matrix(orthographic);
}

void renderer::draw(batch_map& batch_map, const control_mesh& mesh, float x, float y, float depth)
{
    render_batch* target_batch = nullptr;
    switch (mesh.type)
    {
    case ELEMENT_MESH_TYPE_BLOCK: {
        if (batch_map.block_batch == nullptr)
        {
            target_batch = allocate_batch(ELEMENT_MESH_TYPE_BLOCK, batch_map.scissor, nullptr);
            batch_map.block_batch = target_batch;
        }
        else
        {
            target_batch = batch_map.block_batch;
        }
        break;
    }
    case ELEMENT_MESH_TYPE_TEXT: {
        auto iter = batch_map.text_batch.find(mesh.texture);
        if (iter == batch_map.text_batch.end())
        {
            target_batch = allocate_batch(ELEMENT_MESH_TYPE_TEXT, batch_map.scissor, mesh.texture);
            batch_map.text_batch[mesh.texture] = target_batch;
        }
        else
        {
            target_batch = iter->second;
        }
        break;
    }
    case ELEMENT_MESH_TYPE_IMAGE: {
        auto iter = batch_map.image_batch.find(mesh.texture);
        if (iter == batch_map.image_batch.end())
        {
            target_batch = allocate_batch(ELEMENT_MESH_TYPE_IMAGE, batch_map.scissor, mesh.texture);
            batch_map.image_batch[mesh.texture] = target_batch;
        }
        else
        {
            target_batch = iter->second;
        }
        break;
    }
    default:
        break;
    }

    std::uint32_t vertex_base = target_batch->vertex_position.size();
    std::uint32_t index_base = target_batch->indices.size();
    target_batch->indices.resize(target_batch->indices.size() + mesh.index_count);
    std::transform(
        mesh.indices,
        mesh.indices + mesh.index_count,
        target_batch->indices.begin() + index_base,
        [vertex_base](std::uint32_t index) { return index + vertex_base; });

    target_batch->vertex_position.insert(
        target_batch->vertex_position.end(),
        mesh.position,
        mesh.position + mesh.vertex_count);
    target_batch->vertex_uv.insert(
        target_batch->vertex_uv.end(),
        mesh.uv,
        mesh.uv + mesh.vertex_count);
    target_batch->vertex_color.insert(
        target_batch->vertex_color.end(),
        mesh.color,
        mesh.color + mesh.vertex_count);
    target_batch->vertex_offset_index.insert(
        target_batch->vertex_offset_index.end(),
        mesh.vertex_count,
        m_offset.size());

    m_offset.push_back({x, y, depth, 1.0f});
}

void renderer::begin_draw(graphics::mesh_render& mesh)
{
    mesh.submeshes.clear();
    mesh.materials.clear();
}

void renderer::end_draw(graphics::mesh_render& mesh)
{
    m_pipeline->set_offset(m_offset);

    std::size_t vertex_offset = 0;
    std::size_t index_offset = 0;

    for (auto& batch : m_batch_pool)
    {
        m_vertex_buffers[0]->upload(
            batch->vertex_position.data(),
            batch->vertex_position.size() * sizeof(math::float2),
            vertex_offset * sizeof(math::float2));
        m_vertex_buffers[1]->upload(
            batch->vertex_uv.data(),
            batch->vertex_uv.size() * sizeof(math::float2),
            vertex_offset * sizeof(math::float2));
        m_vertex_buffers[2]->upload(
            batch->vertex_color.data(),
            batch->vertex_color.size() * sizeof(std::uint32_t),
            vertex_offset * sizeof(std::uint32_t));
        m_vertex_buffers[3]->upload(
            batch->vertex_offset_index.data(),
            batch->vertex_offset_index.size() * sizeof(std::uint32_t),
            vertex_offset * sizeof(std::uint32_t));
        m_index_buffer->upload(
            batch->indices.data(),
            batch->indices.size() * sizeof(std::uint32_t),
            index_offset * sizeof(std::uint32_t));

        graphics::submesh submesh = {
            .index_start = index_offset,
            .index_end = index_offset + batch->indices.size(),
            .vertex_base = vertex_offset};
        mesh.submeshes.push_back(submesh);

        auto material_parameter = allocate_material_parameter();
        material_parameter->mesh_type(batch->type);
        if (batch->type != ELEMENT_MESH_TYPE_BLOCK)
            material_parameter->texture(batch->texture);

        graphics::material material = {};
        material.pipeline = m_pipeline.get();
        material.parameter = material_parameter->interface();
        material.scissor = graphics::scissor_extent{
            .min_x = static_cast<std::uint32_t>(batch->scissor.x),
            .min_y = static_cast<std::uint32_t>(batch->scissor.y),
            .max_x = static_cast<std::uint32_t>(batch->scissor.x + batch->scissor.width),
            .max_y = static_cast<std::uint32_t>(batch->scissor.y + batch->scissor.height)};

        mesh.materials.push_back(material);

        vertex_offset += batch->vertex_position.size();
        index_offset += batch->indices.size();
    }

    m_material_parameter_counter = 0;
}

render_batch* renderer::allocate_batch(
    control_mesh_type type,
    const node_rect& scissor,
    graphics::rhi_resource* texture)
{
    if (m_batch_pool_index >= m_batch_pool.size())
        m_batch_pool.push_back(std::make_unique<render_batch>());

    auto result = m_batch_pool[m_batch_pool_index].get();
    ++m_batch_pool_index;

    result->type = type;
    result->scissor = scissor;
    result->texture = texture;

    return result;
}

material_pipeline_parameter* renderer::allocate_material_parameter()
{
    if (m_material_parameter_counter >= m_material_parameter_pool.size())
        m_material_parameter_pool.push_back(std::make_unique<material_pipeline_parameter>());

    auto result = m_material_parameter_pool[m_material_parameter_counter].get();
    ++m_material_parameter_counter;

    return result;
}
} // namespace violet::ui
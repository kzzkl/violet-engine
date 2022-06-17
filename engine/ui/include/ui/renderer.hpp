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
    struct batch_key
    {
        element_mesh_type type;
        graphics::resource* texture;

        bool operator==(const batch_key& other) const noexcept
        {
            return type == other.type && texture == other.texture;
        }
    };

    struct batch_hash
    {
        std::size_t operator()(const batch_key& key) const
        {
            std::size_t type_hash = static_cast<std::size_t>(key.type);
            std::size_t texture_hash = std::hash<graphics::resource*>()(key.texture);
            type_hash ^= texture_hash + 0x9e3779b9 + (type_hash << 6) + (type_hash >> 2);
            return type_hash;
        }
    };

    struct batch_map
    {
        element_extent scissor;
        std::unordered_map<batch_key, render_batch*, batch_hash> map;
    };

    void draw(batch_map& batch_map, const element_mesh& mesh, float x, float y, float depth);

    render_batch* allocate_batch();

    std::vector<math::float4> m_offset; // x, y, depth

    std::size_t m_batch_pool_index;
    std::vector<std::unique_ptr<render_batch>> m_batch_pool;
};
} // namespace ash::ui
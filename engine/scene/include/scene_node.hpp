#pragma once

#include "math.hpp"
#include "scene_exports.hpp"
#include <vector>

namespace ash::scene
{
/*class transform;
struct transform_node
{
    math::float4x4 parent_matrix;
    math::float4x4 world_matrix;

    transform* transform;

    std::size_t parent;
    std::vector<std::size_t> children;
};

class transform;
class SCENE_API scene_node
{
public:
    using transform_type = transform;

public:
    scene_node(transform_type* transform);

    void parent(scene_node* parent);
    scene_node* parent() const noexcept { return m_parent; }

    const std::vector<scene_node*>& children() const noexcept { return m_children; }

    bool in_scene() const noexcept { return m_in_scene; }
    void in_scene(bool in_scene) noexcept { m_in_scene = in_scene; }

    std::size_t sync_count() const noexcept { return m_sync_count; }
    void reset_sync_count() noexcept { m_sync_count = 0; }

    bool dirty() const noexcept { return m_dirty; }

    void mark_dirty() noexcept;
    void mark_sync() noexcept;

    transform_type* transform() const noexcept { return m_transform; }
    void transform(transform_type* transform) noexcept { m_transform = transform; }

    math::float4x4 to_world;
    math::float4x4 to_parent;

    bool in_view;

private:
    void add_child(scene_node* child);
    void remove_child(scene_node* child);

    void update_in_scene(bool in_scene);

    scene_node* m_parent;
    std::vector<scene_node*> m_children;

    bool m_in_scene;

    bool m_dirty;
    std::size_t m_sync_count;

    transform_type* m_transform;
};*/
} // namespace ash::scene
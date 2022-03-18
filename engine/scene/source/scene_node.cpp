#include "scene_node.hpp"

namespace ash::scene
{
scene_node::scene_node()
    : to_world(math::matrix::identity<math::float4x4>()),
      to_parent(math::matrix::identity<math::float4x4>()),
      dirty(true),
      updated(false),
      in_scene(false),
      m_parent(nullptr)
{
}

void scene_node::set_parent(scene_node* parent)
{
    if (m_parent == parent)
        return;

    if (m_parent != nullptr)
        m_parent->remove_child(this);

    if (parent != nullptr)
        parent->add_child(this);

    m_parent = parent;
}

void scene_node::add_child(scene_node* child)
{
    m_children.push_back(child);
}

void scene_node::remove_child(scene_node* child)
{
    for (auto iter = m_children.begin(); iter != m_children.end(); ++iter)
    {
        if (*iter == child)
        {
            std::swap(*iter, m_children.back());
            m_children.pop_back();
            break;
        }
    }
}
} // namespace ash::scene
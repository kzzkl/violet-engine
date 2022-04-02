#include "scene_node.hpp"

namespace ash::scene
{
scene_node::scene_node(transform_type* transform)
    : to_world(math::matrix_plain::identity()),
      to_parent(math::matrix_plain::identity()),
      in_view(true),
      m_parent(nullptr),
      m_in_scene(false),
      m_dirty(false),
      m_sync_count(0),
      m_transform(transform)
{
}

void scene_node::parent(scene_node* parent)
{
    if (m_parent == parent)
        return;

    if (m_parent != nullptr)
        m_parent->remove_child(this);

    if (parent != nullptr)
    {
        parent->add_child(this);

        if (parent->in_scene() != m_in_scene)
            update_in_scene(parent->in_scene());
    }
    else
    {
        if (m_in_scene == true)
            update_in_scene(false);
    }

    m_parent = parent;
}

void scene_node::mark_dirty() noexcept
{
    m_dirty = true;
}

void scene_node::mark_sync() noexcept
{
    m_dirty = false;
    ++m_sync_count;
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

void scene_node::update_in_scene(bool in_scene)
{
    m_in_scene = in_scene;

    for (auto child : m_children)
        child->update_in_scene(in_scene);
}
} // namespace ash::scene
#pragma once

#include "ecs/world.hpp"
#include "graphics_interface.hpp"
#include "ui/element.hpp"
#include <unordered_map>
#include <vector>

namespace ash::ui
{
class element_tree
{
public:
    element_tree();

    void link(element& child, element& parent);

    /**
     * @brief Update mesh data.
     * @return Whether mesh data update has occurred.
     */
    bool tick();

    ecs::entity root() const noexcept { return m_root; }

    auto begin() const { return m_meshes.begin(); }
    auto end() const { return m_meshes.end(); }

    void resize(std::uint32_t width, std::uint32_t height);

private:
    struct mesh_key
    {
        element_control_type type;
        graphics::resource* texture;

        bool operator==(const mesh_key& other) const noexcept
        {
            return type == other.type && texture == other.texture;
        }
    };

    struct mesh_hash
    {
        std::size_t operator()(const mesh_key& key) const
        {
            std::size_t type_hash = static_cast<std::size_t>(key.type);
            std::size_t texture_hash = std::hash<graphics::resource*>()(key.texture);
            type_hash ^= texture_hash + 0x9e3779b9 + (type_hash << 6) + (type_hash >> 2);
            return type_hash;
        }
    };

    std::unordered_map<mesh_key, element_mesh, mesh_hash> m_meshes;

    ecs::entity m_root;
    ecs::view<element>* m_view;

    std::unique_ptr<layout> m_layout;

    float m_window_width;
    float m_window_height;
};
} // namespace ash::ui
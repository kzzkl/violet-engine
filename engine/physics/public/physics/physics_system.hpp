#pragma once

#include "algorithm/hash.hpp"
#include "components/collider_component.hpp"
#include "core/engine.hpp"
#include "physics/physics_context.hpp"
#include "physics/physics_scene.hpp"

namespace violet
{
#ifdef VIOLET_PHYSICS_DEBUG_DRAW
class physics_debug;
#endif

class physics_plugin;
class physics_system : public system
{
public:
    physics_system();
    virtual ~physics_system();

    bool initialize(const dictionary& config) override;

    physics_context* get_context() const noexcept
    {
        return m_context.get();
    }

private:
    struct shape_key
    {
        phy_collision_shape_type type;
        float data[3];

        bool operator==(const shape_key& other) const noexcept
        {
            return type == other.type && data[0] == other.data[0] && data[1] == other.data[1] &&
                   data[2] == other.data[2];
        }
    };

    struct shape_hash
    {
        std::uint64_t operator()(const shape_key& key) const noexcept
        {
            return hash::city_hash_64(&key, sizeof(shape_key));
        }
    };

    struct compound_shape_key
    {
        std::vector<shape_key> children;
        std::vector<mat4f> offset;

        bool operator==(const compound_shape_key& other) const noexcept
        {
            if (children.size() != other.children.size())
            {
                return false;
            }

            for (std::size_t i = 0; i < children.size(); ++i)
            {
                if (children[i] != other.children[i] || offset[i] != other.offset[i])
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct compound_shape_hash
    {
        std::uint64_t operator()(const compound_shape_key& key) const noexcept
        {
            return hash::combine(
                hash::city_hash_64(key.children.data(), key.children.size() * sizeof(shape_key)),
                hash::city_hash_64(key.offset.data(), key.offset.size() * sizeof(mat4f)));
        }
    };

    void simulation();

    void update_rigidbody();
    void update_joint();
    void update_transform(entity e, const mat4f& parent_world, bool parent_dirty);

    physics_scene* get_scene(std::uint32_t layer);

    shape_key get_shape_key(const collider_shape& shape);
    phy_collision_shape* get_shape(const collider_shape& shape);
    phy_collision_shape* get_shape(const std::vector<collider_shape>& shapes);

    std::vector<std::unique_ptr<physics_scene>> m_scenes;
    std::unique_ptr<physics_plugin> m_plugin;
    std::unique_ptr<physics_context> m_context;

    std::unordered_map<shape_key, phy_ptr<phy_collision_shape>, shape_hash> m_shapes;
    std::unordered_map<compound_shape_key, phy_ptr<phy_collision_shape>, compound_shape_hash>
        m_compound_shapes;

    std::uint32_t m_system_version{0};

    float m_time{0.0f};

#ifdef VIOLET_PHYSICS_DEBUG_DRAW
    std::unique_ptr<physics_debug> m_debug;
#endif
};
} // namespace violet
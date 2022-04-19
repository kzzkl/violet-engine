#pragma once

#include "entity.hpp"
#include "graphics.hpp"
#include "mmd_component.hpp"
#include "physics.hpp"
#include "pmx_loader.hpp"
#include "scene.hpp"
#include "vmd_loader.hpp"

namespace ash::sample::mmd
{
struct mmd_resource
{
    std::unique_ptr<ash::graphics::resource> vertex_buffer;
    std::unique_ptr<ash::graphics::resource> index_buffer;
    std::vector<std::pair<std::size_t, std::size_t>> submesh;

    std::vector<std::unique_ptr<ash::graphics::resource>> textures;
    std::vector<std::unique_ptr<ash::graphics::render_parameter>> materials;
    std::unique_ptr<ash::graphics::render_parameter> object_parameter;

    std::vector<std::unique_ptr<ash::physics::collision_shape_interface>> collision_shapes;
};

class mmd_loader
{
public:
    mmd_loader(
        ecs::world& world,
        graphics::graphics& graphics,
        scene::scene& scene,
        physics::physics& physics);

    void initialize();
    bool load(
        ecs::entity entity,
        mmd_resource& resource,
        std::string_view pmx,
        std::string_view vmd);

private:
    void load_hierarchy(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_mesh(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_texture(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_material(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_ik(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_physics(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);

    void load_animation(
        ecs::entity entity,
        mmd_resource& resource,
        const pmx_loader& pmx_loader,
        const vmd_loader& vmd_loader);

    std::vector<std::unique_ptr<ash::graphics::resource>> m_internal_toon;
    std::unique_ptr<ash::graphics::render_pipeline> m_pipeline;

    ecs::world& m_world;
    graphics::graphics& m_graphics;
    scene::scene& m_scene;
    physics::physics& m_physics;
};
} // namespace ash::sample::mmd
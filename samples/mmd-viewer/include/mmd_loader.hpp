#pragma once

#include "ecs/entity.hpp"
#include "mmd_pipeline.hpp"
#include "physics_interface.hpp"

namespace ash::sample::mmd
{
struct mmd_resource
{
    std::vector<std::unique_ptr<graphics::resource_interface>> vertex_buffers;
    std::unique_ptr<graphics::resource_interface> index_buffer;
    std::vector<std::pair<std::size_t, std::size_t>> submesh;

    std::vector<std::unique_ptr<graphics::resource_interface>> textures;
    std::vector<std::unique_ptr<material_pipeline_parameter>> materials;

    std::vector<std::unique_ptr<physics::collision_shape_interface>> collision_shapes;
};

class pmx_loader;
class vmd_loader;
class mmd_loader
{
public:
    mmd_loader();

    void initialize();
    bool load(
        ecs::entity entity,
        mmd_resource& resource,
        std::string_view pmx,
        std::string_view vmd,
        graphics::render_pipeline* render_pipeline,
        graphics::skin_pipeline* skin_pipeline);

private:
    void load_hierarchy(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_mesh(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_texture(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_material(
        ecs::entity entity,
        mmd_resource& resource,
        const pmx_loader& loader,
        graphics::render_pipeline* render_pipeline);
    void load_physics(ecs::entity entity, mmd_resource& resource, const pmx_loader& loader);
    void load_ik(ecs::entity entity, const pmx_loader& loader);
    void load_morph(ecs::entity entity, const pmx_loader& pmx_loader, const vmd_loader& vmd_loader);
    void load_animation(
        ecs::entity entity,
        const pmx_loader& pmx_loader,
        const vmd_loader& vmd_loader);

    std::vector<std::unique_ptr<graphics::resource_interface>> m_internal_toon;
};
} // namespace ash::sample::mmd
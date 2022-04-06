#pragma once

#include "context.hpp"
#include "graphics.hpp"
#include "physics.hpp"
#include "pmx_loader.hpp"

namespace ash::sample::mmd
{
struct mmd_resource
{
    std::unique_ptr<ash::graphics::resource> vertex_buffer;
    std::unique_ptr<ash::graphics::resource> index_buffer;

    std::vector<std::unique_ptr<ash::graphics::resource>> textures;
    std::vector<std::unique_ptr<ash::graphics::render_parameter>> materials;
    std::unique_ptr<ash::graphics::render_parameter> object_parameter;

    std::vector<std::pair<std::size_t, std::size_t>> submesh;

    ash::ecs::entity root;
    std::vector<ash::ecs::entity> hierarchy;

    std::vector<std::unique_ptr<ash::physics::collision_shape_interface>> collision_shapes;
};

class mmd_viewer : public ash::core::submodule
{
public:
    static constexpr ash::uuid id = "851e8502-97d6-e3de-2d6f-51075a46deb4";

public:
    mmd_viewer() : submodule("mmd_viewer") {}

    virtual bool initialize(const dictionary& config) override;

    ash::ecs::entity load_mmd(std::string_view name, std::string_view path);

private:
    void load_mesh(mmd_resource& resource, const pmx_loader& loader);
    void load_texture(mmd_resource& resource, const pmx_loader& loader);
    void load_material(mmd_resource& resource, const pmx_loader& loader);
    void load_hierarchy(mmd_resource& resource, const pmx_loader& loader);
    void load_physics(mmd_resource& resource, const pmx_loader& loader);

    std::map<std::string, mmd_resource> m_resources;

    std::vector<std::unique_ptr<ash::graphics::resource>> m_internal_toon;

    std::unique_ptr<ash::graphics::render_pipeline> m_pipeline;
};
} // namespace ash::sample::mmd
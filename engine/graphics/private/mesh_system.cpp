#include "mesh_system.hpp"
#include "components/mesh_component.hpp"
#include "components/mesh_component_meta.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"

namespace violet
{
mesh_system::mesh_system()
    : system("mesh")
{
}

bool mesh_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<mesh_component>();
    world.register_component<mesh_component_meta>();

    return true;
}

void mesh_system::update(render_scene_manager& scene_manager)
{
    auto& world = get_world();

    world.get_view()
        .read<scene_component>()
        .read<mesh_component>()
        .read<transform_world_component>()
        .write<mesh_component_meta>()
        .each(
            [&](const scene_component& scene,
                const mesh_component& mesh,
                const transform_world_component& transform,
                mesh_component_meta& mesh_meta)
            {
                render_scene* render_scene = scene_manager.get_scene(scene.layer);
                if (mesh_meta.scene != render_scene)
                {
                    if (mesh_meta.scene != nullptr)
                    {
                        for (render_id instance : mesh_meta.instances)
                        {
                            mesh_meta.scene->remove_instance(instance);
                        }

                        mesh_meta.scene->remove_mesh(mesh_meta.mesh);
                    }

                    mesh_meta.mesh = render_scene->add_mesh();
                    mesh_meta.instances.clear();
                    mesh_meta.scene = render_scene;
                }

                render_scene->set_mesh_matrix(mesh_meta.mesh, transform.matrix, transform.scale);
            },
            [this](auto& view)
            {
                return view.template is_updated<scene_component>(m_system_version) ||
                       view.template is_updated<transform_world_component>(m_system_version);
            });

    world.get_view().read<mesh_component>().write<mesh_component_meta>().each(
        [](const mesh_component& mesh, mesh_component_meta& mesh_meta)
        {
            if (mesh_meta.scene == nullptr)
            {
                return;
            }

            std::size_t instance_count =
                std::min(mesh.submeshes.size(), mesh_meta.instances.size());

            for (std::size_t i = 0; i < instance_count; ++i)
            {
                mesh_meta.scene->set_instance_geometry(
                    mesh_meta.instances[i],
                    mesh.geometry,
                    mesh.submeshes[i].index);

                mesh_meta.scene->set_instance_material(
                    mesh_meta.instances[i],
                    mesh.submeshes[i].material);
            }

            for (std::size_t i = instance_count; i < mesh.submeshes.size(); ++i)
            {
                render_id instance = mesh_meta.scene->add_instance(mesh_meta.mesh);
                mesh_meta.scene->set_instance_geometry(
                    instance,
                    mesh.geometry,
                    mesh.submeshes[i].index);
                mesh_meta.scene->set_instance_material(instance, mesh.submeshes[i].material);
                mesh_meta.instances.push_back(instance);
            }

            while (mesh_meta.instances.size() > mesh.submeshes.size())
            {
                mesh_meta.scene->remove_instance(mesh_meta.instances.back());
                mesh_meta.instances.pop_back();
            }
        },
        [this](auto& view)
        {
            return view.template is_updated<mesh_component>(m_system_version);
        });

    m_system_version = world.get_version();
}
} // namespace violet
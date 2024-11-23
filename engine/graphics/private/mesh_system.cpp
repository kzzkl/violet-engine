#include "graphics/mesh_system.hpp"
#include "components/mesh.hpp"
#include "components/mesh_render_data.hpp"
#include "components/scene_layer.hpp"
#include "components/transform.hpp"
#include "graphics/graphics_system.hpp"

namespace violet
{
namespace
{
render_instance get_render_instance(const mesh::submesh& submesh) noexcept
{
    return render_instance{
        .vertex_offset = submesh.vertex_offset,
        .index_offset = submesh.index_offset,
        .index_count = submesh.index_count,
        .material = submesh.material,
    };
}
} // namespace

mesh_system::mesh_system()
    : engine_system("mesh")
{
}

bool mesh_system::initialize(const dictionary& config)
{
    task_graph& task_graph = get_task_graph();
    task_group& post_update = task_graph.get_group("Post Update Group");

    task& update_transform = task_graph.get_task("Update Transform");

    task_graph.add_task()
        .set_name("Update Mesh")
        .set_group(post_update)
        .add_dependency(update_transform)
        .set_execute(
            [this]()
            {
                update_render_scene();
                update_render_data();
                m_system_version = get_world().get_version();
            });

    auto& world = get_world();
    world.register_component<mesh>();
    world.register_component<mesh_render_data>();

    return true;
}

void mesh_system::update_render_scene()
{
    auto& world = get_world();

    world.get_view()
        .read<scene_layer>()
        .read<mesh>()
        .read<transform_world>()
        .write<mesh_render_data>()
        .each(
            [](const scene_layer& layer,
               const mesh& mesh,
               const transform_world& transform,
               mesh_render_data& render_data)
            {
                render_scene* scene = render_context::instance().get_scene(layer.scene->get_id());
                if (render_data.scene == scene)
                {
                    return;
                }

                if (render_data.scene != nullptr)
                {
                    for (render_id instance : render_data.instances)
                    {
                        render_data.scene->remove_instance(instance);
                    }

                    render_data.scene->remove_mesh(render_data.mesh);
                }

                render_data.scene = scene;

                render_data.mesh = scene->add_mesh({
                    .model_matrix = transform.matrix,
                    .geometry = mesh.geometry,
                });

                for (auto& submesh : mesh.submeshes)
                {
                    render_id instance =
                        scene->add_instance(render_data.mesh, get_render_instance(submesh));
                    render_data.instances.push_back(instance);
                }

                render_data.scene = scene;
            },
            [this](auto& view)
            {
                return view.is_updated<scene_layer>(m_system_version);
            });
}

void mesh_system::update_render_data()
{
    auto& world = get_world();

    world.get_view().read<mesh>().write<mesh_render_data>().each(
        [](const mesh& mesh, mesh_render_data& render_data)
        {
            if (render_data.scene == nullptr)
            {
                return;
            }

            std::size_t instance_count =
                std::min(mesh.submeshes.size(), render_data.instances.size());

            for (std::size_t i = 0; i < instance_count; ++i)
            {
                render_data.scene->update_instance(
                    render_data.instances[i],
                    get_render_instance(mesh.submeshes[i]));
            }

            for (std::size_t i = instance_count; i < mesh.submeshes.size(); ++i)
            {
                render_id instance_id = render_data.scene->add_instance(
                    render_data.mesh,
                    get_render_instance(mesh.submeshes[i]));
                render_data.instances.push_back(instance_id);
            }

            while (render_data.instances.size() > instance_count)
            {
                render_data.scene->remove_instance(render_data.instances.back());
                render_data.instances.pop_back();
            }
        },
        [this](auto& view)
        {
            return view.is_updated<mesh>(m_system_version);
        });

    world.get_view().read<transform_world>().write<mesh_render_data>().each(
        [](const transform_world& transform, mesh_render_data& render_data)
        {
            if (render_data.scene == nullptr)
            {
                return;
            }

            render_data.scene->update_mesh_model_matrix(render_data.mesh, transform.matrix);
        },
        [this](auto& view)
        {
            return view.is_updated<transform_world>(m_system_version);
        });
}
} // namespace violet
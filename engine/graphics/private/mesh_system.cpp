#include "graphics/mesh_system.hpp"
#include "components/mesh.hpp"
#include "components/mesh_parameter.hpp"
#include "components/transform.hpp"

namespace violet
{
mesh_system::mesh_system()
    : engine_system("Mesh")
{
}

bool mesh_system::initialize(const dictionary& config)
{
    world& world = get_world();

    world.register_component<mesh>();
    world.register_component<mesh_parameter>();

    get_taskflow()
        .add_task(
            [this]()
            {
                add_mesh_parameter();
                remove_mesh_parameter();
                update_mesh_parameter();
                m_system_version = get_world().get_version();
            })
        .set_name("Update Mesh")
        .add_predecessor("Update Transform");

    return true;
}

void mesh_system::add_mesh_parameter()
{
    world& world = get_world();

    std::vector<entity> entities;

    world.get_view().read<entity>().read<mesh>().without<mesh_parameter>().each(
        [&entities](const entity& e, const mesh& mesh)
        {
            entities.push_back(e);
        });

    for (auto& e : entities)
    {
        world.add_component<mesh_parameter>(e);

        auto& parameter = world.get_component<mesh_parameter>(e);
        parameter.parameter = render_device::instance().create_parameter(shader::camera);
    }
}

void mesh_system::remove_mesh_parameter()
{
    world& world = get_world();

    world_command cmd;

    world.get_view().read<entity>().read<mesh_parameter>().without<mesh>().each(
        [&cmd](const entity& e, const mesh_parameter& parameter)
        {
            cmd.remove_component<mesh_parameter>(e);
        });

    world.execute(cmd);
}

void mesh_system::update_mesh_parameter()
{
    get_world().get_view().read<transform_world>().write<mesh_parameter>().each(
        [](const transform_world& transform, mesh_parameter& parameter)
        {
            shader::mesh_data data = {};

            data.model_matrix = transform.matrix;

            matrix4 normal = math::load(transform.matrix);
            normal = matrix::inverse(normal);
            normal = matrix::transpose(normal);
            math::store(normal, data.normal_matrix);

            parameter.parameter->set_uniform(0, &data, sizeof(shader::mesh_data), 0);
        },
        [this](auto& view)
        {
            return view.is_updated<transform_world>(m_system_version);
        });
}
} // namespace violet
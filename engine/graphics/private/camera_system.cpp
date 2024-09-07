#include "graphics/camera_system.hpp"
#include "components/camera.hpp"
#include "components/camera_parameter.hpp"
#include "components/skybox.hpp"
#include "components/transform.hpp"

namespace violet
{
camera_system::camera_system()
    : engine_system("Camera")
{
}

bool camera_system::initialize(const dictionary& config)
{
    world& world = get_world();

    world.register_component<camera>();
    world.register_component<camera_parameter>();
    world.register_component<skybox>();

    get_taskflow()
        .add_task(
            [this]()
            {
                add_camera_parameter();
                remove_camera_parameter();
                update_camera_parameter();
                m_system_version = get_world().get_version();
            })
        .set_name("Update Camera")
        .add_predecessor("Update Transform");

    return true;
}

void camera_system::add_camera_parameter()
{
    world& world = get_world();

    std::vector<entity> entities;

    world.get_view().read<entity>().read<camera>().without<camera_parameter>().each(
        [&entities](const entity& e, const camera& camera)
        {
            entities.push_back(e);
        });

    for (auto& e : entities)
    {
        world.add_component<camera_parameter>(e);

        auto& parameter = world.get_component<camera_parameter>(e);
        parameter.parameter = render_device::instance().create_parameter(shader::camera);
    }
}

void camera_system::remove_camera_parameter()
{
    world& world = get_world();

    std::vector<entity> entities;

    world.get_view().read<entity>().read<camera_parameter>().without<camera>().each(
        [&entities](const entity& e, const camera_parameter& parameter)
        {
            entities.push_back(e);
        });

    for (auto& e : entities)
    {
        world.remove_component<camera_parameter>(e);
    }
}

void camera_system::update_camera_parameter()
{
    get_world().get_view().read<camera>().read<transform_world>().write<camera_parameter>().each(
        [](const camera& camera, const transform_world& transform, camera_parameter& parameter)
        {
            shader::camera_data data = {};

            matrix4 projection = math::load(camera.projection);
            matrix4 view = matrix::inverse(math::load(transform.matrix));
            matrix4 view_projection = matrix::mul(view, projection);

            data.projection = camera.projection;
            math::store(view, data.view);
            math::store(view_projection, data.view_projection);

            data.position = transform.get_position();

            parameter.parameter->set_uniform(0, &data, sizeof(shader::camera_data), 0);
        },
        [this](auto& view)
        {
            return view.is_updated<camera>(m_system_version) ||
                   view.is_updated<transform_world>(m_system_version);
        });

    get_world().get_view().read<skybox>().write<camera_parameter>().each(
        [](const skybox& skybox, camera_parameter& parameter)
        {
            parameter.parameter->set_texture(1, skybox.texture, skybox.sampler);
        },
        [this](auto& view)
        {
            return view.is_updated<skybox>(m_system_version);
        });
}
} // namespace violet
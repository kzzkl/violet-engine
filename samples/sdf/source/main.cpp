#include "components/camera_component.hpp"
#include "core/engine.hpp"
#include "math/math.hpp"
#include "math/matrix.hpp"
#include "math/quaternion.hpp"
#include "sample/sample_system.hpp"
#include "sdf_renderer.hpp"
#include <imgui.h>

namespace violet
{
class sdf_viewer : public sample_system
{
public:
    sdf_viewer()
        : sample_system("sdf_viewer")
    {
    }

    bool initialize(const dictionary& config) override
    {
        if (!sample_system::initialize(config))
        {
            return false;
        }

        if (config.contains("model"))
        {
            load_options options = 0;
            // options |= LOAD_OPTION_GENERATE_CLUSTERS;
            // options |= LOAD_OPTION_GENERATE_MIPMAPS;
            // options |= LOAD_OPTION_COMPRESS_TEXTURES;
            options |= LOAD_OPTION_GENERATE_DISTANCE_FIELD;

            m_root = load_model(config["model"], options);
        }

        auto& world = get_world();

        auto renderer = std::make_unique<sdf_renderer>();

        auto* geometry = get_geometry(0);
        // renderer->set_sdf(geometry->get_distance_field(), geometry->get_distance_field_bounds());
        m_renderer = renderer.get();

        auto& main_camera = world.get_component<camera_component>(get_camera());
        main_camera.renderer = std::move(renderer);

        return true;
    }

private:
    void tick() override
    {
        ImGui::Begin("SDF Viewer");

        static bool bounds_enable = false;
        if (ImGui::Checkbox("Bounds", &bounds_enable))
        {
            m_renderer->set_bounds_enable(bounds_enable);
        }

        static bool brick_enable = false;
        if (ImGui::Checkbox("Bricks", &brick_enable))
        {
            m_renderer->set_brick_enable(brick_enable);
        }

        // The bounding box is in the local space of the model, the transform
        // places the SDF volume in the world. The raymarch, the depth buffer and
        // the bounds wireframe all follow it.
        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            bool changed = false;
            changed |= ImGui::DragFloat3("Position", &m_position.x, 0.01f);
            changed |= ImGui::DragFloat3("Rotation", &m_rotation.x, 0.5f);
            changed |= ImGui::SliderFloat3("Scale", &m_scale.x, 0.05f, 4.0f);

            if (ImGui::Button("Reset"))
            {
                m_position = {0.0f, 0.0f, 0.0f};
                m_rotation = {0.0f, 0.0f, 0.0f};
                m_scale = {1.0f, 1.0f, 1.0f};

                changed = true;
            }

            if (changed)
            {
                vec3f rotation{
                    math::to_radians(m_rotation.x),
                    math::to_radians(m_rotation.y),
                    math::to_radians(m_rotation.z)};

                m_renderer->set_matrix(
                    matrix::affine_transform(m_scale, quaternion::from_euler(rotation), m_position));
            }
        }

        ImGui::End();
    }

    entity m_root;

    sdf_renderer* m_renderer;

    // SDF transform, the rotation is in degrees.
    vec3f m_position{0.0f, 0.0f, 0.0f};
    vec3f m_rotation{0.0f, 0.0f, 0.0f};
    vec3f m_scale{1.0f, 1.0f, 1.0f};
};
} // namespace violet

int main()
{
    using namespace violet;

    application app("assets/config/sdf.json");
    app.install<sdf_viewer>();
    app.run();

    return 0;
}
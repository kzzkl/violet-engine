#include "mmd_viewer.hpp"
#include "common/utility.hpp"
#include "components/atmosphere_component.hpp"
#include "components/camera_component.hpp"
#include "components/first_person_control_component.hpp"
#include "components/light_component.hpp"
#include "components/mesh_component.hpp"
#include "components/mmd_skeleton_component.hpp"
#include "components/morph_component.hpp"
#include "components/scene_component.hpp"
#include "components/transform_component.hpp"
#include "control/control_system.hpp"
#include "gf2/gf2_material.hpp"
#include "graphics/graphics_system.hpp"
#include "graphics/renderers/features/gtao_feature.hpp"
#include "graphics/renderers/features/taa_feature.hpp"
#include "math/matrix.hpp"
#include "mmd_animation.hpp"
#include "mmd_material.hpp"
#include "physics/physics_system.hpp"
#include "sample/imgui_system.hpp"
#include "scene/transform_system.hpp"
#include "tools/texture_tool.hpp"
#include "window/window_system.hpp"
#include <imgui.h>


namespace violet
{
mmd_viewer::mmd_viewer()
    : system("mmd_viewer")
{
}

mmd_viewer::~mmd_viewer() {}

void mmd_viewer::install(application& app)
{
    app.install<window_system>();
    app.install<graphics_system>();
    app.install<physics_system>();
    app.install<control_system>();
    app.install<imgui_system>();
    app.install<mmd_animation>();

    m_app = &app;
}

bool mmd_viewer::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<mmd_skeleton_component>();

    auto& window = get_system<window_system>();
    window.on_resize().add_task().set_execute(
        [this]()
        {
            resize();
        });
    window.on_destroy().add_task().set_execute(
        [this]()
        {
            m_app->exit();
        });

    auto& task_graph = get_task_graph();
    auto& update = task_graph.get_group("Update");

    task_graph.add_task()
        .set_name("MMD Tick")
        .set_group(update)
        .set_options(TASK_OPTION_MAIN_THREAD)
        .set_execute(
            [this]()
            {
                update_sdf();
                draw_imgui();
            });

    initialize_render();

    std::filesystem::path pmx_path = string_to_wstring(config["pmx"]);
    std::filesystem::path vmd_path =
        config.find("vmd") == config.end() ? L"" : string_to_wstring(config["vmd"]);
    std::filesystem::path skybox_path =
        config.find("skybox") == config.end() ? L"" : string_to_wstring(config["skybox"]);
    initialize_scene(pmx_path, vmd_path, skybox_path);

    std::filesystem::path name = pmx_path.filename().replace_extension();
    if (config.find(name) != config.end())
    {
        override_material(config[name], pmx_path.parent_path().string());
    }

    resize();

    return true;
}

void mmd_viewer::initialize_render()
{
    m_swapchain = render_device::instance().create_swapchain({
        .flags = RHI_TEXTURE_TRANSFER_DST | RHI_TEXTURE_RENDER_TARGET,
        .window_handle = get_system<window_system>().get_handle(),
    });

    for (std::uint32_t i = 1; i <= 10; ++i)
    {
        texture_data data = {
            .format = RHI_FORMAT_R8G8B8A8_SRGB,
        };

        texture_tool::load(std::format("assets/mmd/toon{:02}.bmp", i), data);
        m_internal_toons.push_back(std::make_unique<texture_2d>(data));
    }
}

void mmd_viewer::initialize_scene(
    const std::filesystem::path& pmx_path,
    const std::filesystem::path& vmd_path,
    const std::filesystem::path& skybox_path)
{
    auto& world = get_world();

    // if (!skybox_path.empty())
    // {
    //     m_skybox = std::make_unique<skybox>(skybox_path.string());

    //     entity scene_skybox = world.create();
    //     world.add_component<transform_component, skybox_component,
    //     scene_component>(scene_skybox); auto& skybox =
    //     world.get_component<skybox_component>(scene_skybox); skybox.skybox = m_skybox.get();
    // }

    m_camera = world.create();
    world.add_component<
        transform_component,
        camera_component,
        first_person_control_component,
        scene_component>(m_camera);

    auto& camera_transform = world.get_component<transform_component>(m_camera);
    camera_transform.set_position({0.0f, 0.0f, -40.0f});

    auto& camera = world.get_component<camera_component>(m_camera);
    camera.render_target = m_swapchain.get();
    camera.renderer = std::make_unique<deferred_renderer_imgui>();
    camera.background = BACKGROUND_TYPE_ATMOSPHERE;

    m_light = world.create();
    world
        .add_component<light_component, transform_component, scene_component, atmosphere_component>(
            m_light);

    auto& light_transform = world.get_component<transform_component>(m_light);
    light_transform.lookat({-1.0f, -1.0f, 1.0f});

    auto& light = world.get_component<light_component>(m_light);
    light.type = LIGHT_DIRECTIONAL;
    light.color = {3.0f, 3.0f, 3.0f};
    light.cast_shadow = true;

    std::vector<texture_2d*> internal_toons(m_internal_toons.size());
    std::transform(
        m_internal_toons.begin(),
        m_internal_toons.end(),
        internal_toons.begin(),
        [](auto& texture)
        {
            return texture.get();
        });

    mmd_loader loader(internal_toons);
    if (auto result = loader.load(pmx_path.string(), vmd_path.string(), get_world()))
    {
        m_model = std::move(*result);
    }
}

void mmd_viewer::override_material(const dictionary& info, const std::filesystem::path& root_path)
{
    std::size_t material_offset = m_model.materials.size();

    if (info["shading"] == "gf2")
    {
        for (const auto& gf2_material : info["materials"])
        {
            load_gf2_material(gf2_material, root_path);
        }

        // Get face bone for updating SDF direction.
        if (info.find("face_bone_index") != info.end())
        {
            auto& skeleton = get_world().get_component<mmd_skeleton_component>(m_model.root);
            m_face = skeleton.bones[info["face_bone_index"]].entity;
        }
    }

    auto& mesh = get_world().get_component<mesh_component>(m_model.root);
    for (std::size_t i = 0; i < info["submesh_material"].size(); ++i)
    {
        std::size_t material_index = info["submesh_material"][i];
        mesh.submeshes[i].material = m_model.materials[material_index + material_offset].get();
    }

    if (info.find("outline") != info.end())
    {
        auto outline_width = info["outline"].find("width");
        if (outline_width != info["outline"].end())
        {
            for (const auto& material : m_model.outline_materials)
            {
                auto* outline_material = static_cast<mmd_outline_material*>(material.get());
                if (outline_material != nullptr)
                {
                    outline_material->set_width(*outline_width);
                }
            }
        }

        auto outline_strength = info["outline"].find("strength");
        if (outline_strength != info["outline"].end())
        {
            for (const auto& material : m_model.outline_materials)
            {
                auto* outline_material = static_cast<mmd_outline_material*>(material.get());
                if (outline_material != nullptr)
                {
                    outline_material->set_strength(*outline_strength);
                }
            }
        }
    }
}

void mmd_viewer::load_gf2_material(const dictionary& info, const std::filesystem::path& root_path)
{
    auto load_texture = [&](const std::string& texture, bool srgb) -> texture_2d*
    {
        auto path = root_path / texture;

        texture_data data;
        data.format = srgb ? RHI_FORMAT_R8G8B8A8_SRGB : RHI_FORMAT_R8G8B8A8_UNORM;
        texture_tool::load(path.string(), data);

        m_model.textures.push_back(std::make_unique<texture_2d>(data));
        return m_model.textures.back().get();
    };

    std::string type = info["type"];
    if (type == "base")
    {
        auto material = std::make_unique<gf2_material_base>();
        material->set_diffuse(load_texture(info["diffuse"], true));
        material->set_normal(load_texture(info["normal"], false));
        material->set_rmo(load_texture(info["rmo"], false));
        material->set_ramp(load_texture(info["ramp"], false));
        m_model.materials.push_back(std::move(material));
    }
    else if (type == "face")
    {
        auto material = std::make_unique<gf2_material_face>();
        material->set_diffuse(load_texture(info["diffuse"], true));
        material->set_ramp(load_texture(info["ramp"], false));
        material->set_sdf(load_texture(info["sdf"], false));

        m_sdf_callback =
            [face_material =
                 material.get()](const vec3f& face_front_dir, const vec3f& face_left_dir)
        {
            face_material->set_face_dir(face_front_dir, face_left_dir);
        };

        m_model.materials.push_back(std::move(material));
    }
    else if (type == "eye")
    {
        auto material = std::make_unique<gf2_material_eye>();
        material->set_diffuse(load_texture(info["diffuse"], true));
        m_model.materials.push_back(std::move(material));
    }
    else if (type == "eye blend")
    {
        auto material = std::make_unique<gf2_material_eye_blend>(info["operator"] == "add");
        material->set_blend(load_texture(info["blend"], true));
        m_model.materials.push_back(std::move(material));
    }
    else if (type == "hair")
    {
        auto material = std::make_unique<gf2_material_hair>();
        material->set_diffuse(load_texture(info["diffuse"], true));
        material->set_specular(load_texture(info["specular"], false));
        material->set_ramp(load_texture(info["ramp"], false));
        m_model.materials.push_back(std::move(material));
    }
    else if (type == "plush")
    {
        auto material = std::make_unique<gf2_material_plush>();
        material->set_diffuse(load_texture(info["diffuse"], true));
        material->set_normal(load_texture(info["normal"], false));
        material->set_noise(load_texture(info["noise"], false));
        material->set_ramp(load_texture(info["ramp"], false));
        m_model.materials.push_back(std::move(material));
    }
}

void mmd_viewer::update_sdf()
{
    if (m_face != INVALID_ENTITY && m_sdf_callback)
    {
        mat4f transform = get_system<transform_system>().get_world_matrix(m_face);
        vec4f face_front_dir = matrix::mul(vec4f{0.0f, 0.0f, -1.0f, 0.0f}, transform);
        vec4f face_left_dir = matrix::mul(vec4f{1.0f, 0.0f, 0.0f, 0.0f}, transform);
        m_sdf_callback(
            {face_front_dir.x, face_front_dir.y, face_front_dir.z},
            {face_left_dir.x, face_left_dir.y, face_left_dir.z});
    }
}

void mmd_viewer::draw_imgui()
{
    auto& world = get_world();

    if (ImGui::CollapsingHeader("Light"))
    {
        bool dirty = false;

        static vec3f rotation;
        dirty = ImGui::SliderFloat("Rotate X", &rotation.x, -math::PI, math::PI) || dirty;
        dirty = ImGui::SliderFloat("Rotate Y", &rotation.y, 0.0f, math::TWO_PI) || dirty;

        if (dirty)
        {
            auto& transform = world.get_component<transform_component>(m_light);

            vec4f_simd q = quaternion::from_euler(math::load(rotation));
            transform.set_rotation(q);
        }

        static vec3f color = {1.0f, 1.0f, 1.0f};
        static float strength = 3.0f;

        if (ImGui::ColorEdit3("Color", &color.x))
        {
            auto& light = world.get_component<light_component>(m_light);
            light.color = color * strength;
        }

        if (ImGui::SliderFloat("Strength", &strength, 0.0f, 10.0f))
        {
            auto& light = world.get_component<light_component>(m_light);
            light.color = color * strength;
        }
    }

    if (ImGui::CollapsingHeader("Transform"))
    {
        static float rotate = 0.0f;
        if (ImGui::SliderFloat("Rotate", &rotate, 0.0, 360.0))
        {
            auto& transform = world.get_component<transform_component>(m_model.root);
            transform.set_rotation(
                quaternion::from_axis_angle(vec3f{0.0f, 1.0f, 0.0f}, math::to_radians(rotate)));
        }

        static float translate = 0.0f;
        if (ImGui::SliderFloat("Translate", &translate, -5.0f, 5.0))
        {
            auto& transform = world.get_component<transform_component>(m_model.root);
            transform.set_position({0.0f, 0.0f, translate});
        }
    }

    if (ImGui::CollapsingHeader("Material"))
    {
        static float outline_width = 1.0f;
        static float outline_z_offset = 0.0f;
        static float outline_strength = 1.0f;

        bool dirty = false;
        if (ImGui::SliderFloat("Outline Width", &outline_width, 0.0f, 1.0f))
        {
            dirty = true;
        }

        if (ImGui::SliderFloat("Outline Z Offset", &outline_z_offset, 0.0f, 1.0f))
        {
            dirty = true;
        }

        if (ImGui::SliderFloat("Outline Strength", &outline_strength, 0.0f, 1.0f))
        {
            dirty = true;
        }

        if (dirty)
        {
            for (const auto& material : m_model.outline_materials)
            {
                auto* outline_material = static_cast<mmd_outline_material*>(material.get());

                if (outline_material != nullptr)
                {
                    outline_material->set_width(outline_width);
                    outline_material->set_z_offset(outline_z_offset);
                    outline_material->set_strength(outline_strength);
                }
            }
        }
    }

    if (ImGui::CollapsingHeader("Morph"))
    {
        auto& morph = world.get_component<morph_component>(m_model.root);
        for (std::size_t i = 0; i < morph.weights.size(); ++i)
        {
            std::string t = std::to_string(i);
            ImGui::SliderFloat(t.c_str(), &morph.weights[i], 0.0f, 1.0f);
        }
    }

    if (ImGui::CollapsingHeader("TAA"))
    {
        auto& main_camera = get_world().get_component<camera_component>(m_camera);
        auto* taa = main_camera.renderer->get_feature<taa_feature>();

        static bool enable_taa = taa->is_enable();

        ImGui::Checkbox("Enable##TAA", &enable_taa);

        if (enable_taa)
        {
            taa->enable();
        }
        else
        {
            taa->disable();
        }
    }

    if (ImGui::CollapsingHeader("GTAO"))
    {
        auto& main_camera = world.get_component<camera_component>(m_camera);
        auto* gtao = main_camera.renderer->get_feature<gtao_feature>();

        static bool enable_gtao = gtao->is_enable();
        if (ImGui::Checkbox("Enable##GTAO", &enable_gtao))
        {
            if (enable_gtao)
            {
                gtao->enable();
            }
            else
            {
                gtao->disable();
            }
        }

        if (enable_gtao)
        {
            ImGui::SliderInt("Slice Count", reinterpret_cast<int*>(&gtao->slice_count), 1, 5);
            ImGui::SliderInt("Step Count", reinterpret_cast<int*>(&gtao->step_count), 1, 5);
            ImGui::SliderFloat("Radius", &gtao->radius, 0.0f, 10.0f);
            ImGui::SliderFloat("Falloff", &gtao->falloff, 0.1f, 1.0f);
        }
    }
}

void mmd_viewer::resize()
{
    m_swapchain->resize();
}
} // namespace violet
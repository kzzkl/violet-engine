#include "mmd_viewer.hpp"
#include "components/camera.hpp"
#include "components/mesh.hpp"
#include "components/orbit_control.hpp"
#include "components/transform.hpp"
#include "core/engine.hpp"
#include "graphics/graphics_system.hpp"
#include "pmx_loader.hpp"
#include "window/window_system.hpp"

namespace violet::sample
{

mmd_viewer::mmd_viewer() : engine_system("mmd viewer"), m_depth_stencil(nullptr)
{
}

mmd_viewer::~mmd_viewer()
{
    rhi_renderer* rhi = get_system<graphics_system>().get_context()->get_rhi();
    for (auto& [name, model] : m_models)
    {
        for (rhi_resource* texture : model.textures)
            rhi->destroy_texture(texture);
    }

    m_models.clear();

    rhi->destroy_sampler(m_sampler);
}

bool mmd_viewer::initialize(const dictionary& config)
{
    on_tick().then(
        [this](float delta)
        {
            tick(delta);
        });

    get_system<window_system>().on_resize().then(
        [this](std::uint32_t width, std::uint32_t height)
        {
            resize(width, height);
        });

    initialize_render();
    load_model(config["model"]);

    return true;
}

void mmd_viewer::shutdown()
{
}

void mmd_viewer::initialize_render()
{
    auto& graphics = get_system<graphics_system>();
    rhi_renderer* rhi = graphics.get_context()->get_rhi();

    m_render_graph = std::make_unique<mmd_render_graph>(graphics.get_context());
    m_render_graph->compile();

    auto extent = get_system<window_system>().get_extent();
    resize(extent.width, extent.height);

    rhi_sampler_desc sampler_desc = {};
    sampler_desc.min_filter = RHI_FILTER_LINEAR;
    sampler_desc.mag_filter = RHI_FILTER_LINEAR;
    sampler_desc.address_mode_u = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_v = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_desc.address_mode_w = RHI_SAMPLER_ADDRESS_MODE_REPEAT;
    m_sampler = rhi->create_sampler(sampler_desc);

    std::vector<std::string> internal_toon_paths = {
        "mmd-viewer/mmd/toon01.dds",
        "mmd-viewer/mmd/toon02.dds",
        "mmd-viewer/mmd/toon03.dds",
        "mmd-viewer/mmd/toon04.dds",
        "mmd-viewer/mmd/toon05.dds",
        "mmd-viewer/mmd/toon06.dds",
        "mmd-viewer/mmd/toon07.dds",
        "mmd-viewer/mmd/toon08.dds",
        "mmd-viewer/mmd/toon09.dds",
        "mmd-viewer/mmd/toon10.dds"};
    for (const std::string& toon : internal_toon_paths)
        m_internal_toons.push_back(rhi->create_texture(toon.c_str()));
}

void mmd_viewer::load_model(std::string_view path)
{
    pmx_loader loader;
    if (m_models.find(path.data()) != m_models.end() || !loader.load(path))
        return;

    mmd_model& model = m_models[path.data()];
    const pmx_mesh& pmx_mesh = loader.get_mesh();

    model.geometry =
        std::make_unique<geometry>(get_system<graphics_system>().get_context()->get_rhi());
    model.geometry->add_attribute("position", pmx_mesh.position);
    model.geometry->add_attribute("normal", pmx_mesh.normal);
    model.geometry->add_attribute("uv", pmx_mesh.uv);
    model.geometry->set_indices(pmx_mesh.indices);

    rhi_renderer* rhi = get_system<graphics_system>().get_context()->get_rhi();
    for (const std::string& texture : pmx_mesh.textures)
        model.textures.push_back(rhi->create_texture(texture.c_str()));
    for (const pmx_material& pmx_material : pmx_mesh.materials)
    {
        material* material = m_render_graph->add_material("mmd material", pmx_material.name_jp);

        material->set(
            "mmd material",
            mmd_material{
                .diffuse = pmx_material.diffuse,
                .specular = pmx_material.specular,
                .specular_strength = pmx_material.specular_strength,
                .edge_color = pmx_material.edge_color,
                .ambient = pmx_material.ambient,
                .edge_size = pmx_material.edge_size,
                .toon_mode = pmx_material.toon_mode,
                .spa_mode = pmx_material.sphere_mode});

        material->set("mmd tex", model.textures[pmx_material.texture_index], m_sampler);
        if (pmx_material.toon_mode == PMX_TOON_MODE_TEXTURE)
            material->set("mmd toon", model.textures[pmx_material.toon_index], m_sampler);
        else
            material->set("mmd toon", m_internal_toons[pmx_material.toon_index], m_sampler);

        if (pmx_material.sphere_mode != PMX_SPHERE_MODE_DISABLED)
            material->set("mmd spa", model.textures[pmx_material.sphere_index], m_sampler);
        else
            material->set("mmd spa", m_internal_toons[0], m_sampler);

        model.materials.push_back(material);
    }

    model.model = std::make_unique<node>("mmd", get_world());
    auto [transform_ptr, mesh_ptr] = model.model->add_component<transform, mesh>();
    mesh_ptr->set_geometry(model.geometry.get());

    for (auto& submesh : pmx_mesh.submeshes)
    {
        mesh_ptr->add_submesh(
            0,
            pmx_mesh.position.size(),
            submesh.index_start,
            submesh.index_count,
            model.materials[submesh.material_index]);
    }
}

void mmd_viewer::tick(float delta)
{
    get_system<graphics_system>().render(m_render_graph.get());

    // m_camera->get_component<orbit_control>()->phi += delta;
    // m_camera->get_component<orbit_control>()->dirty = true;
}

void mmd_viewer::resize(std::uint32_t width, std::uint32_t height)
{
    rhi_renderer* rhi = get_system<graphics_system>().get_context()->get_rhi();
    rhi->destroy_depth_stencil_buffer(m_depth_stencil);

    rhi_depth_stencil_buffer_desc depth_stencil_buffer_desc = {};
    depth_stencil_buffer_desc.width = width;
    depth_stencil_buffer_desc.height = height;
    depth_stencil_buffer_desc.samples = RHI_SAMPLE_COUNT_1;
    depth_stencil_buffer_desc.format = RHI_RESOURCE_FORMAT_D24_UNORM_S8_UINT;
    m_depth_stencil = rhi->create_depth_stencil_buffer(depth_stencil_buffer_desc);

    if (m_camera)
    {
        auto camera_ptr = m_camera->get_component<camera>();
        camera_ptr->resize(width, height);
        camera_ptr->set_attachment(1, m_depth_stencil);
    }
    else
    {
        m_camera = std::make_unique<node>("main camera", get_world());
        auto [camera_ptr, transform_ptr, orbit_control_ptr] =
            m_camera->add_component<camera, transform, orbit_control>();
        camera_ptr->set_render_pass(m_render_graph->get_render_pass("main"));
        camera_ptr->set_attachment(0, rhi->get_back_buffer(), true);
        camera_ptr->set_attachment(1, m_depth_stencil);
        camera_ptr->resize(width, height);
        transform_ptr->set_position(0.0f, 2.0f, -5.0f);
        orbit_control_ptr->r = 50.0f;
        orbit_control_ptr->target = {0.0f, 15.0f, 0.0f};
    }
}
} // namespace violet::sample
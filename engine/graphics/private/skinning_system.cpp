#include "skinning_system.hpp"
#include "components/mesh_component.hpp"
#include "components/mesh_component_meta.hpp"
#include "components/morph_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/skinned_component.hpp"
#include "components/skinned_component_meta.hpp"
#include "components/transform_component.hpp"
#include "graphics/geometry_manager.hpp"

namespace violet
{
skinning_system::skinning_system()
    : system("skinning")
{
}

bool skinning_system::initialize(const dictionary& config)
{
    auto& world = get_world();
    world.register_component<skinned_component>();
    world.register_component<skinned_component_meta>();
    world.register_component<skeleton_component>();
    world.register_component<morph_component>();

    return true;
}

void skinning_system::update()
{
    update_skin();
    update_skeleton();
    update_morph();

    m_system_version = get_world().get_version();
}

void skinning_system::morphing(rhi_command* command)
{
    if (m_morphing_queue.empty())
    {
        return;
    }

    auto& device = render_device::instance();

    command->begin_label("Morphing");

    command->set_pipeline(device.get_pipeline({
        .compute_shader = device.get_shader<morphing_cs>(),
    }));
    command->set_parameter(0, device.get_bindless_parameter());

    for (const auto& morphing_data : m_morphing_queue)
    {
        morphing_data.morph_target_buffer->update_morph(
            command,
            morphing_data.morph_vertex_buffer,
            std::span(morphing_data.weights, morphing_data.weight_count));
    }

    command->end_label();
}

void skinning_system::skinning(rhi_command* command)
{
    if (m_skinning_queue.empty())
    {
        return;
    }

    auto& device = render_device::instance();
    auto* geometry_manager = device.get_geometry_manager();

    command->begin_label("Skinning");

    for (const auto& skinning_data : m_skinning_queue)
    {
        geometry* original_geometry = skinning_data.original_geometry;
        geometry* skinned_geometry = skinning_data.skinned_geometry;

        skinning_cs::constant_data constant = {
            .position_input_address = geometry_manager->get_buffer_address(
                original_geometry->get_id(),
                GEOMETRY_BUFFER_POSITION),
            .normal_input_address = geometry_manager->get_buffer_address(
                original_geometry->get_id(),
                GEOMETRY_BUFFER_NORMAL),
            .tangent_input_address = geometry_manager->get_buffer_address(
                original_geometry->get_id(),
                GEOMETRY_BUFFER_TANGENT),
            .position_output_address = geometry_manager->get_buffer_address(
                skinned_geometry->get_id(),
                GEOMETRY_BUFFER_POSITION),
            .normal_output_address = geometry_manager->get_buffer_address(
                skinned_geometry->get_id(),
                GEOMETRY_BUFFER_NORMAL),
            .tangent_output_address = geometry_manager->get_buffer_address(
                skinned_geometry->get_id(),
                GEOMETRY_BUFFER_TANGENT),
            .vertex_buffer = geometry_manager->get_vertex_buffer()->get_uav()->get_bindless(),
            .skeleton = skinning_data.skeleton->get_uav()->get_bindless(),
        };

        for (std::size_t i = 0; i < skinning_data.additional_buffers.size(); ++i)
        {
            if (skinning_data.additional_buffers[i] != nullptr)
            {
                constant.additional[i] =
                    skinning_data.additional_buffers[i]->get_srv()->get_bindless();
            }
        }

        command->set_pipeline(device.get_pipeline({
            .compute_shader = skinning_data.shader,
        }));
        command->set_constant(&constant, sizeof(skinning_cs::constant_data));
        command->set_parameter(0, device.get_bindless_parameter());
        command->dispatch((skinning_data.vertex_count + 63) / 64, 1, 1);

        rhi_buffer_barrier barrier = {
            .buffer = geometry_manager->get_vertex_buffer()->get_rhi(),
            .src_stages = RHI_PIPELINE_STAGE_COMPUTE,
            .src_access = RHI_ACCESS_SHADER_WRITE,
            .dst_stages = RHI_PIPELINE_STAGE_VERTEX,
            .dst_access = RHI_ACCESS_SHADER_READ,
        };

        std::vector<rhi_buffer_barrier> buffer_barriers;
        buffer_barriers.reserve(3);

        barrier.offset = constant.position_output_address;
        barrier.size =
            geometry_manager->get_buffer_size(skinned_geometry->get_id(), GEOMETRY_BUFFER_POSITION);
        buffer_barriers.push_back(barrier);

        barrier.offset = constant.normal_output_address;
        barrier.size =
            geometry_manager->get_buffer_size(skinned_geometry->get_id(), GEOMETRY_BUFFER_NORMAL);
        buffer_barriers.push_back(barrier);

        barrier.offset = constant.tangent_output_address;
        barrier.size =
            geometry_manager->get_buffer_size(skinned_geometry->get_id(), GEOMETRY_BUFFER_TANGENT);
        buffer_barriers.push_back(barrier);

        command->set_pipeline_barrier(
            buffer_barriers.data(),
            static_cast<std::uint32_t>(buffer_barriers.size()),
            nullptr,
            0);
    }

    command->end_label();
}

void skinning_system::update_skin()
{
    auto& world = get_world();

    world.get_view()
        .read<mesh_component>()
        .read<mesh_component_meta>()
        .read<skinned_component>()
        .write<skinned_component_meta>()
        .each(
            [](const mesh_component& mesh,
               const mesh_component_meta& mesh_meta,
               const skinned_component& skinned,
               skinned_component_meta& skinned_meta)
            {
                if (skinned_meta.skinned_geometry == nullptr)
                {
                    skinned_meta.skinned_geometry = std::make_unique<geometry>();
                }

                if (mesh.geometry != skinned_meta.original_geometry)
                {
                    skinned_meta.original_geometry = mesh.geometry;

                    geometry* original_geometry = skinned_meta.original_geometry;
                    geometry* skinned_geometry = skinned_meta.skinned_geometry.get();

                    skinned_geometry->set_positions(original_geometry->get_positions());
                    skinned_geometry->set_normals(original_geometry->get_normals());
                    skinned_geometry->set_tangents(original_geometry->get_tangents());
                    skinned_geometry->set_texcoords_shared(original_geometry);

                    for (std::size_t i = 0; i < geometry::max_custom_attribute; ++i)
                    {
                        skinned_geometry->set_custom_shared(i, original_geometry);
                    }

                    skinned_geometry->set_indexes_shared(original_geometry);

                    if (original_geometry->get_morph_target_count() != 0)
                    {
                        skinned_geometry->set_additional_buffer(
                            "morph",
                            nullptr,
                            original_geometry->get_vertex_count() * sizeof(vec3f),
                            RHI_BUFFER_TRANSFER_DST | RHI_BUFFER_STORAGE_TEXEL);
                    }

                    skinned_geometry->clear_submeshes();
                    for (const auto& submesh : original_geometry->get_submeshes())
                    {
                        skinned_geometry->add_submesh(
                            submesh.vertex_offset,
                            submesh.index_offset,
                            submesh.index_count);
                    }
                }

                if (mesh_meta.scene != nullptr)
                {
                    for (std::uint32_t submesh_index = 0; submesh_index < mesh.submeshes.size();
                         ++submesh_index)
                    {
                        mesh_meta.scene->set_instance_geometry(
                            mesh_meta.instances[submesh_index],
                            skinned_meta.skinned_geometry.get(),
                            mesh.submeshes[submesh_index].index);
                    }
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<skinned_component>(m_system_version);
            });

    m_skinning_queue.clear();

    world.get_view().read<skinned_component>().read<skinned_component_meta>().each(
        [this](const skinned_component& skinned, const skinned_component_meta& skinned_meta)
        {
            if (skinned_meta.bone_buffers.empty())
            {
                return;
            }

            skinning_data data = {
                .shader = skinned.shader,
                .vertex_count = skinned_meta.skinned_geometry->get_vertex_count(),
                .skeleton = skinned_meta.bone_buffers[skinned_meta.current_index].get(),
                .original_geometry = skinned_meta.original_geometry,
                .skinned_geometry = skinned_meta.skinned_geometry.get(),
            };

            data.additional_buffers.reserve(skinned.inputs.size());
            for (const auto& input : skinned.inputs)
            {
                if (input == "morph")
                {
                    data.additional_buffers.push_back(
                        skinned_meta.skinned_geometry->get_additional_buffer(input));
                }
                else
                {
                    data.additional_buffers.push_back(
                        skinned_meta.original_geometry->get_additional_buffer(input));
                }
            }

            m_skinning_queue.push_back(data);
        });
}

void skinning_system::update_skeleton()
{
    auto& world = get_world();

    world.get_view()
        .read<transform_world_component>()
        .read<skeleton_component>()
        .write<skinned_component_meta>()
        .each(
            [&world](
                const transform_world_component& transform,
                const skeleton_component& skeleton,
                skinned_component_meta& skinned_meta)
            {
                if (skeleton.bones.empty())
                {
                    return;
                }

                if (skinned_meta.bone_buffers.empty())
                {
                    auto& device = render_device::instance();

                    std::size_t frame_resource_count = device.get_frame_resource_count();
                    for (std::size_t i = 0; i < frame_resource_count; ++i)
                    {
                        skinned_meta.bone_buffers.emplace_back(std::make_unique<structured_buffer>(
                            sizeof(mat4f) * skeleton.bones.size(),
                            RHI_BUFFER_STORAGE | RHI_BUFFER_HOST_VISIBLE));
                    }

                    skinned_meta.current_index = 0;
                }
                else
                {
                    skinned_meta.current_index =
                        (skinned_meta.current_index + 1) % skinned_meta.bone_buffers.size();
                }

                mat4f_simd root_transform_inv = matrix::inverse(math::load(transform.matrix));

                auto* buffer = static_cast<mat4f*>(
                    skinned_meta.bone_buffers[skinned_meta.current_index]->get_buffer_pointer());
                for (const auto& bone : skeleton.bones)
                {
                    const auto& bone_transform =
                        world.get_component<const transform_world_component>(bone.entity);

                    mat4f_simd bone_matrix =
                        matrix::mul(math::load(bone_transform.matrix), root_transform_inv);
                    bone_matrix = matrix::mul(math::load(bone.binding_pose_inv), bone_matrix);

                    math::store(bone_matrix, *buffer);

                    ++buffer;
                }
            });
}

void skinning_system::update_morph()
{
    auto& world = get_world();

    m_morphing_queue.clear();

    world.get_view().write<skinned_component_meta>().read<morph_component>().each(
        [this](skinned_component_meta& skinned_meta, const morph_component& morph)
        {
            morph_target_buffer* morph_target_buffer =
                skinned_meta.original_geometry->get_morph_target_buffer();

            if (morph_target_buffer == nullptr)
            {
                return;
            }

            morphing_data data = {
                .morph_target_buffer = morph_target_buffer,
                .weights = morph.weights.data(),
                .weight_count = static_cast<std::uint32_t>(morph.weights.size()),
                .morph_vertex_buffer =
                    skinned_meta.skinned_geometry->get_additional_buffer("morph"),
            };
            m_morphing_queue.push_back(data);
        });
}
} // namespace violet
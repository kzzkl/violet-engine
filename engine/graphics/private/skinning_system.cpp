#include "skinning_system.hpp"
#include "components/mesh_component.hpp"
#include "components/mesh_meta_component.hpp"
#include "components/morph_component.hpp"
#include "components/skeleton_component.hpp"
#include "components/skinned_component.hpp"
#include "components/skinned_meta_component.hpp"
#include "components/transform_component.hpp"

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
    world.register_component<skinned_meta_component>();
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

    command->begin_label("Skinning");

    for (const auto& skinning_data : m_skinning_queue)
    {
        skinning_cs::skinning_data skinning_constant = {
            .skeleton = skinning_data.skeleton->get_uav()->get_bindless(),
        };
        std::size_t buffer_index = 0;
        for (auto* input : skinning_data.input)
        {
            if (input != nullptr)
            {
                skinning_constant.buffers[buffer_index] = input->get_srv()->get_bindless();
            }

            ++buffer_index;
        }

        std::vector<rhi_buffer_barrier> buffer_barriers;
        buffer_barriers.reserve(skinning_data.output.size());
        for (auto* output : skinning_data.output)
        {
            if (output != nullptr)
            {
                skinning_constant.buffers[buffer_index] = output->get_uav()->get_bindless();
                buffer_barriers.push_back({
                    .buffer = output->get_rhi(),
                    .src_stages = RHI_PIPELINE_STAGE_COMPUTE,
                    .src_access = RHI_ACCESS_SHADER_WRITE,
                    .dst_stages = RHI_PIPELINE_STAGE_VERTEX_INPUT,
                    .dst_access = RHI_ACCESS_VERTEX_ATTRIBUTE_READ,
                    .offset = 0,
                    .size = output->get_size(),
                });
            }

            ++buffer_index;
        }

        rhi_parameter* parameter = device.allocate_parameter(skinning_cs::skinning);
        parameter->set_constant(0, &skinning_constant, sizeof(skinning_cs::skinning_data));

        command->set_pipeline(device.get_pipeline({
            .compute_shader = skinning_data.shader,
        }));
        command->set_parameter(0, device.get_bindless_parameter());
        command->set_parameter(1, parameter);
        command->dispatch((skinning_data.vertex_count + 63) / 64, 1, 1);

        command->set_pipeline_barrier(buffer_barriers.data(), buffer_barriers.size(), nullptr, 0);
    }

    command->end_label();
}

void skinning_system::update_skin()
{
    auto& world = get_world();

    world.get_view()
        .read<mesh_component>()
        .read<mesh_meta_component>()
        .read<skinned_component>()
        .write<skinned_meta_component>()
        .each(
            [](const mesh_component& mesh,
               const mesh_meta_component& mesh_meta,
               const skinned_component& skinned,
               skinned_meta_component& skinned_meta)
            {
                if (skinned_meta.skinned_geometry == nullptr)
                {
                    skinned_meta.skinned_geometry = std::make_unique<geometry>();
                }

                if (mesh.geometry != skinned_meta.original_geometry)
                {
                    skinned_meta.original_geometry = mesh.geometry;

                    std::map<std::string, vertex_buffer*> buffers;
                    for (const auto& [name, buffer] : mesh.geometry->get_vertex_buffers())
                    {
                        buffers[name] = buffer;
                    }

                    for (const auto& [name, format] : skinned.outputs)
                    {
                        skinned_meta.skinned_geometry->add_attribute(
                            name,
                            rhi_get_format_stride(format),
                            mesh.geometry->get_vertex_count(),
                            RHI_BUFFER_VERTEX | RHI_BUFFER_STORAGE);

                        auto iter = buffers.find(name);
                        if (iter != buffers.end())
                        {
                            buffers.erase(iter);
                        }
                    }

                    if (mesh.geometry->get_morph_target_count() != 0)
                    {
                        skinned_meta.skinned_geometry->add_attribute(
                            "morph",
                            sizeof(vec3f),
                            mesh.geometry->get_vertex_count(),
                            RHI_BUFFER_TRANSFER_DST | RHI_BUFFER_STORAGE_TEXEL);
                    }

                    for (const auto& [name, buffer] : buffers)
                    {
                        skinned_meta.skinned_geometry->add_attribute(
                            name,
                            mesh.geometry->get_vertex_buffer(name));
                    }

                    skinned_meta.skinned_geometry->set_indexes(mesh.geometry->get_index_buffer());

                    skinned_meta.skinned_geometry->set_vertex_count(
                        mesh.geometry->get_vertex_count());
                    skinned_meta.skinned_geometry->set_index_count(
                        mesh.geometry->get_index_count());
                }

                if (mesh_meta.scene != nullptr)
                {
                    mesh_meta.scene->set_mesh_geometry(
                        mesh_meta.mesh,
                        skinned_meta.skinned_geometry.get());
                }
            },
            [this](auto& view)
            {
                return view.template is_updated<skinned_component>(m_system_version);
            });

    m_skinning_queue.clear();

    world.get_view().read<skinned_component>().read<skinned_meta_component>().each(
        [this](const skinned_component& skinned, const skinned_meta_component& skinned_meta)
        {
            if (skinned_meta.bone_buffers.empty())
            {
                return;
            }

            skinning_data data = {
                .shader = skinned.shader,
                .vertex_count = skinned_meta.skinned_geometry->get_vertex_count(),
                .skeleton = skinned_meta.bone_buffers[skinned_meta.current_index].get(),
            };

            data.input.reserve(skinned.inputs.size());
            for (const auto& input : skinned.inputs)
            {
                if (input == "morph")
                {
                    data.input.push_back(skinned_meta.skinned_geometry->get_vertex_buffer(input));
                }
                else
                {
                    data.input.push_back(skinned_meta.original_geometry->get_vertex_buffer(input));
                }
            }

            data.output.reserve(skinned.outputs.size());
            for (const auto& [name, format] : skinned.outputs)
            {
                data.output.push_back(skinned_meta.skinned_geometry->get_vertex_buffer(name));
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
        .write<skinned_meta_component>()
        .each(
            [&world](
                const transform_world_component& transform,
                const skeleton_component& skeleton,
                skinned_meta_component& skinned_meta)
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

    world.get_view().write<skinned_meta_component>().read<morph_component>().each(
        [this](skinned_meta_component& skinned_meta, const morph_component& morph)
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
                .weight_count = morph.weights.size(),
                .morph_vertex_buffer = skinned_meta.skinned_geometry->get_vertex_buffer("morph"),
            };
            m_morphing_queue.push_back(data);
        });
}
} // namespace violet
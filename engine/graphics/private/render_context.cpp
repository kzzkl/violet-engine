#include "graphics/render_context.hpp"
#include "components/mesh.hpp"

namespace violet
{
static constexpr std::uint32_t CONSTANT_BUFFER_SIZE = 16 * 1024 * 1024;
static constexpr std::uint32_t STAGING_BUFFER_SIZE = 16 * 1024 * 1024;

render_context::render_context()
    : m_constant_allocator(CONSTANT_BUFFER_SIZE),
      m_staging_allocator(STAGING_BUFFER_SIZE)
{
    auto& device = render_device::instance();

    m_light = device.create_parameter(shader::light);

    rhi_buffer_desc staging_buffer_desc = {
        .data = nullptr,
        .size = STAGING_BUFFER_SIZE,
        .flags = RHI_BUFFER_TRANSFER_SRC | RHI_BUFFER_HOST_VISIBLE};
    for (std::size_t i = 0; i < device.get_frame_resource_count(); ++i)
    {
        m_staging_buffers.push_back(device.create_buffer(staging_buffer_desc));
    }
}

render_list render_context::get_render_list(const render_camera& camera) const
{
    render_list list = {};
    list.camera = camera.parameter;
    list.light = m_light.get();

    struct material_info
    {
        std::vector<std::size_t> pipelines;
        std::vector<std::size_t> parameters;
    };
    std::unordered_map<material*, material_info> material_infos;
    std::unordered_map<std::uint64_t, std::size_t> pipeline_map;

    for (auto& [mesh, parameter] : m_meshes)
    {
        std::size_t mesh_index = list.meshes.size();
        list.meshes.push_back({parameter, mesh->geometry});

        for (auto& submesh : mesh->submeshes)
        {
            auto info = material_infos[submesh.material];
            if (info.pipelines.empty())
            {
                for (auto& [pipeline, parameter] : submesh.material->get_passes())
                {
                    std::uint64_t hash = hash::city_hash_64(&pipeline, sizeof(rdg_render_pipeline));
                    auto iter = pipeline_map.find(hash);
                    if (iter == pipeline_map.end())
                    {
                        std::size_t batch_index = pipeline_map[hash] = list.batches.size();
                        info.pipelines.push_back(batch_index);
                        info.parameters.push_back(0);
                        list.batches.push_back({pipeline, {parameter.get()}});
                    }
                    else
                    {
                        info.pipelines.push_back(iter->second);
                        info.parameters.push_back(list.batches[iter->second].parameters.size());
                        list.batches[iter->second].parameters.push_back(parameter.get());
                    }
                }
            }

            for (std::size_t i = 0; i < info.pipelines.size(); ++i)
            {
                list.batches[info.pipelines[i]].items.push_back(
                    {mesh_index,
                     info.parameters[i],
                     submesh.vertex_start,
                     submesh.index_start,
                     submesh.index_count});
            }
        }
    }
    return list;
}

allocation render_context::allocate_constant(std::size_t size)
{
    return m_constant_allocator.allocate(size);
}

void render_context::free_constant(allocation allocation)
{
    m_constant_allocator.free(allocation);
}

void render_context::update_constant(std::uint32_t offset, const void* data, std::size_t size)
{
    std::size_t frame_resource_index = render_device::instance().get_frame_resource_index();

    allocation allocation = m_staging_allocator.allocate(size);

    update_buffer_command command = {
        .src = m_staging_buffers[frame_resource_index].get(),
        .dst = m_constant_buffer.get(),
        .src_offset = allocation.offset,
        .dst_offset = offset,
        .size = size};

    std::memcpy(command.src->get_buffer(), data, size);

    m_update_buffer_commands.push_back(command);
}

void render_context::update_resource()
{
    if (m_update_buffer_commands.empty())
    {
        return;
    }

    auto& device = render_device::instance();

    rhi_command* command = device.allocate_command();

    for (auto& update : m_update_buffer_commands)
    {
        rhi_buffer_region src_region = {.offset = update.src_offset, .size = update.size};
        rhi_buffer_region dst_region = {.offset = update.dst_offset, .size = update.size};

        command->copy_buffer(update.src, src_region, update.dst, dst_region);
    }
}
} // namespace violet
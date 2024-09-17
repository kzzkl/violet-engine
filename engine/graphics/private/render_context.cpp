#include "graphics/render_context.hpp"
#include "components/mesh.hpp"

namespace violet
{
render_context::render_context()
{
    m_light = render_device::instance().create_parameter(shader::light);
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
                auto& passes = submesh.material->get_passes();
                for (std::size_t i = 0; i < passes.size(); ++i)
                {
                    std::uint64_t hash =
                        hash::city_hash_64(&passes[i], sizeof(rdg_render_pipeline));
                    auto iter = pipeline_map.find(hash);
                    if (iter == pipeline_map.end())
                    {
                        std::size_t batch_index = pipeline_map[hash] = list.batches.size();
                        info.pipelines.push_back(batch_index);
                        info.parameters.push_back(0);
                        list.batches.push_back({passes[i], {submesh.material->get_parameter(i)}});
                    }
                    else
                    {
                        info.pipelines.push_back(iter->second);
                        info.parameters.push_back(list.batches[iter->second].parameters.size());
                        list.batches[iter->second].parameters.push_back(
                            submesh.material->get_parameter(i));
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
} // namespace violet
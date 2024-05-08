#include "graphics/render_graph/render_graph.hpp"
#include "render_graph/pass_batch.hpp"
#include <cassert>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>

namespace violet
{
std::vector<pass*> toposort(const std::unordered_map<pass*, std::vector<pass*>>& dependency)
{
    std::unordered_map<pass*, std::size_t> in_edge;
    for (auto& [node, edges] : dependency)
        in_edge[node] = 0;
    for (auto& [node, edges] : dependency)
    {
        for (pass* edge : edges)
            ++in_edge[edge];
    }

    std::stack<pass*> stack;
    for (auto& [node, in_count] : in_edge)
    {
        if (in_count == 0)
            stack.push(node);
    }

    std::vector<pass*> result;
    while (!stack.empty())
    {
        pass* node = stack.top();
        stack.pop();

        auto iter = dependency.find(node);
        if (iter != dependency.end())
        {
            for (pass* edge : iter->second)
            {
                --in_edge[edge];
                if (in_edge[edge] == 0)
                    stack.push(edge);
            }
        }
        result.push_back(node);
    }

    return result;
}

render_graph::render_graph(renderer* renderer) noexcept : m_renderer(renderer)
{
}

render_graph::~render_graph()
{
}

void render_graph::add_edge(
    resource* resource,
    pass* pass,
    std::string_view pass_reference_name,
    edge_operate operate)
{
    auto reference = pass->get_reference(pass_reference_name);

    reference->resource = resource;
    if (operate == EDGE_OPERATE_DONT_CARE)
        reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
    else if (operate == EDGE_OPERATE_CLEAR)
        reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
    else if (operate == EDGE_OPERATE_STORE)
        reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
}

void render_graph::add_edge(
    pass* src,
    std::string_view src_reference_name,
    pass* dst,
    std::string_view dst_reference_name,
    edge_operate operate)
{
    auto src_reference = src->get_reference(src_reference_name);
    auto dst_reference = dst->get_reference(dst_reference_name);

    if (src_reference->type == PASS_REFERENCE_TYPE_ATTACHMENT)
    {
        if (operate == EDGE_OPERATE_DONT_CARE || operate == EDGE_OPERATE_CLEAR)
            src_reference->attachment.store_op = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        else if (operate == EDGE_OPERATE_STORE)
            src_reference->attachment.store_op = RHI_ATTACHMENT_STORE_OP_STORE;
    }

    if (dst_reference->type == PASS_REFERENCE_TYPE_ATTACHMENT)
    {
        if (operate == EDGE_OPERATE_DONT_CARE)
            dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        else if (operate == EDGE_OPERATE_CLEAR)
            dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        else if (operate == EDGE_OPERATE_STORE)
            dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
    }

    if (src_reference->type != PASS_REFERENCE_TYPE_BUFFER &&
        dst_reference->type != PASS_REFERENCE_TYPE_BUFFER)
    {
        auto& next_layout = src_reference->type == PASS_REFERENCE_TYPE_TEXTURE
                                ? src_reference->texture.next_layout
                                : src_reference->attachment.next_layout;

        next_layout = dst_reference->type == PASS_REFERENCE_TYPE_TEXTURE
                          ? dst_reference->texture.layout
                          : dst_reference->attachment.layout;
    }

    m_edges.emplace_back(std::make_unique<edge>(src, src_reference, dst, dst_reference));
}

void render_graph::add_material_layout(std::string_view name, const std::vector<mesh_pass*>& passes)
{
    auto [iter, result] =
        m_material_layouts.insert(std::make_pair(name, std::make_unique<material_layout>(passes)));

    assert(result);
}

material* render_graph::add_material(std::string_view name, std::string_view layout_name)
{
    material_layout* layout = m_material_layouts[layout_name.data()].get();
    auto [iter, result] =
        m_materials.insert(std::make_pair(name, std::make_unique<material>(layout)));

    assert(result);
    return iter->second.get();
}

material* render_graph::get_material(std::string_view name) const
{
    return m_materials.at(name.data()).get();
}

void render_graph::remove_material(std::string_view name)
{
    m_materials.erase(name.data());
}

void render_graph::compile()
{
    bind_resource();
    dead_stripping();

    auto passes = merge_pass();

    for (std::size_t i = 0; i < passes.size(); ++i)
    {
        if (passes[i][0]->get_flags() & PASS_FLAG_RENDER)
            m_batchs.push_back(std::make_unique<render_pass_batch>(passes[i], m_renderer));
    }

    m_used_semaphores.resize(m_renderer->get_frame_resource_count());
}

rhi_semaphore* render_graph::execute()
{
    switch_frame_resource();

    std::vector<rhi_semaphore*> wait_semaphores;
    wait_semaphores.reserve(m_swapchains.size());
    std::vector<rhi_semaphore*> signal_semaphores;
    signal_semaphores.reserve(m_swapchains.size() + 1);
    for (std::size_t i = 0; i < m_swapchains.size(); ++i)
    {
        signal_semaphores.push_back(allocate_semaphore());
        wait_semaphores.push_back(m_swapchains[i]->acquire_texture());
    }
    signal_semaphores.push_back(allocate_semaphore());

    rhi_render_command* command = m_renderer->allocate_command();

    for (auto& batch : m_batchs)
        batch->execute(command, nullptr);

    m_renderer->execute({command}, signal_semaphores, wait_semaphores, nullptr);

    for (std::size_t i = 0; i < m_swapchains.size(); ++i)
        m_swapchains[i]->present(signal_semaphores[i]);

    return signal_semaphores.back();
}

void render_graph::bind_resource()
{
    std::unordered_map<pass*, std::vector<pass*>> dependency;
    for (auto& edge : m_edges)
        dependency[edge->get_src()].push_back(edge->get_dst());

    for (pass* pass : toposort(dependency))
    {
        for (auto& edge : m_edges)
        {
            if (edge->get_src() != pass)
                continue;

            pass_reference* src_reference = edge->get_src_reference();
            pass_reference* dst_reference = edge->get_dst_reference();

            dst_reference->resource = src_reference->resource;
        }
    }
}

void render_graph::dead_stripping()
{
    std::queue<pass*> queue;
    for (auto& pass : m_passes)
    {
        for (pass_reference* reference : pass->get_references(PASS_ACCESS_FLAG_WRITE))
        {
            if (reference->resource && reference->resource->is_external())
                queue.push(pass.get());
        }
    }

    std::unordered_map<pass*, std::vector<pass*>> in_edges;
    for (auto& edge : m_edges)
        in_edges[edge->get_dst()].push_back(edge->get_src());

    std::set<pass*> useful_pass;
    while (!queue.empty())
    {
        pass* pass = queue.front();
        queue.pop();

        for (auto dependency_pass : in_edges[pass])
            queue.push(dependency_pass);

        useful_pass.insert(pass);
    }

    auto pass_iter = std::remove_if(
        m_passes.begin(),
        m_passes.end(),
        [&useful_pass](auto& pass)
        {
            return useful_pass.find(pass.get()) == useful_pass.end();
        });
    m_passes.erase(pass_iter, m_passes.end());

    auto edge_iter = std::remove_if(
        m_edges.begin(),
        m_edges.end(),
        [&useful_pass](auto& edge)
        {
            return useful_pass.find(edge->get_src()) == useful_pass.end() ||
                   useful_pass.find(edge->get_dst()) == useful_pass.end();
        });
    m_edges.erase(edge_iter, m_edges.end());
}

std::vector<std::vector<pass*>> render_graph::merge_pass()
{
    std::unordered_map<pass*, pass*> dsu;
    for (auto& pass : m_passes)
        dsu[pass.get()] = pass.get();

    auto get_root = [&dsu](pass* p) -> pass*
    {
        pass* result = dsu[p];
        while (result != dsu[result])
            result = dsu[result];
        return result;
    };

    std::unordered_map<pass*, std::vector<pass*>> split_map;
    for (auto& edge : m_edges)
    {
        if (edge->get_operate() == EDGE_OPERATE_CLEAR)
            split_map[edge->get_src()].push_back(edge->get_dst());
    }

    std::unordered_map<pass*, std::vector<pass*>> pass_dependency;
    for (auto& edge : m_edges)
    {
        auto& split_passes = split_map[edge->get_src()];
        bool split = std::find(split_passes.begin(), split_passes.end(), edge->get_dst()) !=
                     split_passes.end();

        if (edge->get_src_reference()->type == PASS_REFERENCE_TYPE_ATTACHMENT &&
            edge->get_dst_reference()->type == PASS_REFERENCE_TYPE_ATTACHMENT && !split)
        {
            pass* src_root = get_root(edge->get_src());
            pass* dst_root = get_root(edge->get_dst());

            dsu[dst_root] = src_root;
            pass_dependency[edge->get_src()].push_back(edge->get_dst());
        }
    }

    std::unordered_map<pass*, std::vector<pass*>> batchs;
    for (auto& pass : m_passes)
        batchs[get_root(pass.get())].push_back(pass.get());

    for (auto& [root, batch] : batchs)
    {
        std::unordered_map<pass*, std::vector<pass*>> graph;
        for (pass* pass : batch)
            graph[pass] = pass_dependency[pass];

        batch = toposort(graph);
    }

    std::unordered_map<pass*, std::vector<pass*>> batch_dependency;
    for (auto& edge : m_edges)
    {
        if (edge->get_dst_reference()->type != PASS_REFERENCE_TYPE_ATTACHMENT)
            batch_dependency[get_root(edge->get_src())].push_back(get_root(edge->get_dst()));
    }

    std::vector<std::vector<pass*>> result;
    for (pass* root : toposort(batch_dependency))
        result.push_back(batchs[root]);

    return result;
}

void render_graph::switch_frame_resource()
{
    auto& finish_semaphores = m_used_semaphores[m_renderer->get_frame_resource_index()];
    for (rhi_semaphore* samphore : finish_semaphores)
        m_free_semaphores.push_back(samphore);
    finish_semaphores.clear();
}

rhi_semaphore* render_graph::allocate_semaphore()
{
    if (m_free_semaphores.empty())
    {
        m_semaphores.emplace_back(m_renderer->create_semaphore());
        m_free_semaphores.push_back(m_semaphores.back().get());
    }

    rhi_semaphore* semaphore = m_free_semaphores.back();
    m_used_semaphores[m_renderer->get_frame_resource_index()].push_back(semaphore);

    m_free_semaphores.pop_back();

    return semaphore;
}
} // namespace violet
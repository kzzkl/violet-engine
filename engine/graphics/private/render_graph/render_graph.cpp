#include "graphics/render_graph/render_graph.hpp"
#include "render_graph/rdg_pass_batch.hpp"
#include <cassert>
#include <iostream>
#include <queue>
#include <set>
#include <stack>
#include <unordered_map>

namespace violet
{
std::vector<rdg_pass*> toposort(
    const std::unordered_map<rdg_pass*, std::vector<rdg_pass*>>& dependency)
{
    std::unordered_map<rdg_pass*, std::size_t> in_edge;
    for (auto& [node, edges] : dependency)
        in_edge[node] = 0;
    for (auto& [node, edges] : dependency)
    {
        for (rdg_pass* edge : edges)
            ++in_edge[edge];
    }

    std::stack<rdg_pass*> stack;
    for (auto& [node, in_count] : in_edge)
    {
        if (in_count == 0)
            stack.push(node);
    }

    std::vector<rdg_pass*> result;
    while (!stack.empty())
    {
        rdg_pass* node = stack.top();
        stack.pop();

        auto iter = dependency.find(node);
        if (iter != dependency.end())
        {
            for (rdg_pass* edge : iter->second)
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

render_graph::render_graph() noexcept
{
}

render_graph::~render_graph()
{
}

void render_graph::add_edge(
    rdg_resource* resource,
    rdg_pass* pass,
    std::size_t reference_index,
    rdg_edge_operate operate)
{
    auto reference = pass->get_reference(reference_index);

    reference->resource = resource;
    if (operate == RDG_EDGE_OPERATE_DONT_CARE)
        reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
    else if (operate == RDG_EDGE_OPERATE_CLEAR)
        reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
    else if (operate == RDG_EDGE_OPERATE_STORE)
        reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
}

void render_graph::add_edge(
    rdg_pass* src,
    std::size_t src_reference_index,
    rdg_pass* dst,
    std::size_t dst_reference_index,
    rdg_edge_operate operate)
{
    auto src_reference = src->get_reference(src_reference_index);
    auto dst_reference = dst->get_reference(dst_reference_index);

    if (src_reference->type == RDG_PASS_REFERENCE_TYPE_ATTACHMENT)
    {
        if (operate == RDG_EDGE_OPERATE_DONT_CARE || operate == RDG_EDGE_OPERATE_CLEAR)
            src_reference->attachment.store_op = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        else if (operate == RDG_EDGE_OPERATE_STORE)
            src_reference->attachment.store_op = RHI_ATTACHMENT_STORE_OP_STORE;
    }

    if (dst_reference->type == RDG_PASS_REFERENCE_TYPE_ATTACHMENT)
    {
        if (operate == RDG_EDGE_OPERATE_DONT_CARE)
            dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
        else if (operate == RDG_EDGE_OPERATE_CLEAR)
            dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
        else if (operate == RDG_EDGE_OPERATE_STORE)
            dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
    }

    if (src_reference->type != RDG_PASS_REFERENCE_TYPE_BUFFER &&
        dst_reference->type != RDG_PASS_REFERENCE_TYPE_BUFFER)
    {
        auto& next_layout = src_reference->type == RDG_PASS_REFERENCE_TYPE_TEXTURE
                                ? src_reference->texture.next_layout
                                : src_reference->attachment.next_layout;

        next_layout = dst_reference->type == RDG_PASS_REFERENCE_TYPE_TEXTURE
                          ? dst_reference->texture.layout
                          : dst_reference->attachment.layout;
    }

    m_edges.emplace_back(std::make_unique<rdg_edge>(src, src_reference, dst, dst_reference));
}

void render_graph::compile(render_device* device)
{
    bind_resource();
    // dead_stripping();

    for (std::size_t i = 0; i < m_resources.size(); ++i)
        m_resources[i]->m_index = i;

    for (std::size_t i = 0; i < m_passes.size(); ++i)
        m_passes[i]->m_index = i;

    auto passes = merge_pass();

    for (std::size_t i = 0; i < passes.size(); ++i)
    {
        if (passes[i][0]->get_type() == RDG_PASS_TYPE_RENDER)
            m_batchs.push_back(std::make_unique<rdg_render_pass_batch>(passes[i], device));
        else if (passes[i][0]->get_type() == RDG_PASS_TYPE_COMPUTE)
            m_batchs.push_back(std::make_unique<rdg_compute_pass_batch>(passes[i], device));
        else if (passes[i][0]->get_type() == RDG_PASS_TYPE_OTHER)
            m_batchs.push_back(std::make_unique<rdg_other_pass_batch>(passes[i], device));
    }

    for (auto& resource : m_resources)
        m_resource_indices[resource->get_name()] = resource->get_index();
    for (auto& pass : m_passes)
        m_pass_indices[pass->get_name()] = pass->get_index();

    m_semaphores.resize(device->get_frame_resource_count());

    m_device = device;
}

void render_graph::execute(rhi_command* command, rdg_context* context)
{
    for (auto& batch : m_batchs)
        batch->execute(command, context);
}

std::unique_ptr<rdg_context> render_graph::create_context()
{
    std::vector<rdg_pass*> passes;
    for (auto& pass : m_passes)
        passes.push_back(pass.get());

    return std::make_unique<rdg_context>(m_resources.size(), passes, m_device);
}

std::size_t render_graph::get_resource_index(std::string_view name) const
{
    return m_resource_indices.at(name.data());
}

rdg_resource_type render_graph::get_resource_type(std::size_t index) const
{
    return m_resources[index]->get_type();
}

std::size_t render_graph::get_pass_index(std::string_view name) const
{
    return m_pass_indices.at(name.data());
}

void render_graph::bind_resource()
{
    std::unordered_map<rdg_pass*, std::vector<rdg_pass*>> dependency;
    for (auto& edge : m_edges)
        dependency[edge->get_src()].push_back(edge->get_dst());

    for (rdg_pass* pass : toposort(dependency))
    {
        for (auto& edge : m_edges)
        {
            if (edge->get_src() != pass)
                continue;

            rdg_pass_reference* src_reference = edge->get_src_reference();
            rdg_pass_reference* dst_reference = edge->get_dst_reference();

            dst_reference->resource = src_reference->resource;
        }
    }
}

void render_graph::dead_stripping()
{
    std::queue<rdg_pass*> queue;
    for (auto& pass : m_passes)
    {
        for (rdg_pass_reference* reference : pass->get_references(RDG_PASS_ACCESS_FLAG_WRITE))
        {
            if (reference->resource && reference->resource->is_external())
                queue.push(pass.get());
        }
    }

    std::unordered_map<rdg_pass*, std::vector<rdg_pass*>> in_edges;
    for (auto& edge : m_edges)
        in_edges[edge->get_dst()].push_back(edge->get_src());

    std::set<rdg_pass*> useful_pass;
    while (!queue.empty())
    {
        rdg_pass* pass = queue.front();
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

std::vector<std::vector<rdg_pass*>> render_graph::merge_pass()
{
    std::unordered_map<rdg_pass*, rdg_pass*> dsu;
    for (auto& pass : m_passes)
        dsu[pass.get()] = pass.get();

    auto get_root = [&dsu](rdg_pass* p) -> rdg_pass*
    {
        rdg_pass* result = dsu[p];
        while (result != dsu[result])
            result = dsu[result];
        return result;
    };

    std::unordered_map<rdg_pass*, std::vector<rdg_pass*>> split_map;
    for (auto& edge : m_edges)
    {
        if (edge->get_operate() == RDG_EDGE_OPERATE_CLEAR)
            split_map[edge->get_src()].push_back(edge->get_dst());
    }

    std::unordered_map<rdg_pass*, std::vector<rdg_pass*>> pass_dependency;
    for (auto& edge : m_edges)
    {
        auto& split_passes = split_map[edge->get_src()];
        bool split = std::find(split_passes.begin(), split_passes.end(), edge->get_dst()) !=
                     split_passes.end();
        if (split || edge->get_src()->get_type() != edge->get_dst()->get_type())
            continue;

        if ((edge->get_src_reference()->type == RDG_PASS_REFERENCE_TYPE_ATTACHMENT &&
             edge->get_dst_reference()->type == RDG_PASS_REFERENCE_TYPE_ATTACHMENT) ||
            edge->get_src()->get_type() != RDG_PASS_TYPE_RENDER)
        {
            rdg_pass* src_root = get_root(edge->get_src());
            rdg_pass* dst_root = get_root(edge->get_dst());

            dsu[dst_root] = src_root;
            pass_dependency[edge->get_src()].push_back(edge->get_dst());
        }
    }

    std::unordered_map<rdg_pass*, std::vector<rdg_pass*>> batchs;
    for (auto& pass : m_passes)
        batchs[get_root(pass.get())].push_back(pass.get());

    for (auto& [root, batch] : batchs)
    {
        std::unordered_map<rdg_pass*, std::vector<rdg_pass*>> graph;
        for (rdg_pass* pass : batch)
            graph[pass] = pass_dependency[pass];

        batch = toposort(graph);
    }

    std::unordered_map<rdg_pass*, std::vector<rdg_pass*>> batch_dependency;
    for (auto& pass : m_passes)
        batch_dependency[get_root(pass.get())] = {};

    for (auto& edge : m_edges)
    {
        if (edge->get_dst_reference()->type != RDG_PASS_REFERENCE_TYPE_ATTACHMENT)
            batch_dependency[get_root(edge->get_src())].push_back(get_root(edge->get_dst()));
    }

    std::vector<std::vector<rdg_pass*>> result;
    for (rdg_pass* root : toposort(batch_dependency))
        result.push_back(batchs[root]);

    return result;
}
} // namespace violet
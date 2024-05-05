#include "graphics/render_graph/render_graph.hpp"
#include "render_graph/pass_batch.hpp"
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

    if (operate == EDGE_OPERATE_DONT_CARE)
    {
        src_reference->attachment.store_op = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
    else if (operate == EDGE_OPERATE_CLEAR)
    {
        src_reference->attachment.store_op = RHI_ATTACHMENT_STORE_OP_DONT_CARE;
        dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_CLEAR;
    }
    else if (operate == EDGE_OPERATE_STORE)
    {
        src_reference->attachment.store_op = RHI_ATTACHMENT_STORE_OP_STORE;
        dst_reference->attachment.load_op = RHI_ATTACHMENT_LOAD_OP_LOAD;
    }

    m_edges.emplace_back(std::make_unique<edge>(src, src_reference, dst, dst_reference));
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
}

void render_graph::execute(rhi_render_command* command)
{
    for (auto& batch : m_batchs)
        batch->execute(command, nullptr);
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
} // namespace violet
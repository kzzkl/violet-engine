#pragma once

#include <cassert>
#include <memory>
#include <queue>
#include <stack>
#include <vector>

namespace violet
{
template <typename G>
class directed_graph_iterator
{
public:
    using graph_type = G;

public:
    directed_graph_iterator() : m_graph(nullptr), m_index(0) {}

    directed_graph_iterator(graph_type* graph, std::size_t index) : m_graph(graph), m_index(index)
    {
    }

    std::size_t operator-(directed_graph_iterator other) const noexcept
    {

        return m_index - other.m_index;
    }

    bool operator==(directed_graph_iterator other) const noexcept
    {
        return m_graph == other.m_graph && m_index == other.m_index;
    }

    bool operator!=(directed_graph_iterator other) const noexcept { return !operator==(other); }

    graph_type* get_graph() const noexcept { return m_graph; }

protected:
    graph_type* m_graph;
    std::size_t m_index;

private:
    template <typename V, typename E>
    friend class directed_graph;
};

template <typename G>
class directed_graph_vertex_iterator : public directed_graph_iterator<G>
{
public:
    using graph_type = G;
    using vertex_type = typename graph_type::vertex_type;
    using base_type = directed_graph_iterator<G>;

    using base_type::base_type;

public:
    vertex_type* operator->() { return this->m_graph->m_vertices[this->m_index].vertex.get(); }

    vertex_type& operator*() { return *operator->(); }
};

template <typename G>
class directed_graph_edge_iterator : public directed_graph_iterator<G>
{
public:
    using graph_type = G;
    using edge_type = typename graph_type::edge_type;
    using base_type = directed_graph_iterator<G>;

    using base_type::base_type;

public:
    edge_type* operator->() { return this->m_graph->m_edges[this->m_index].edge.get(); }

    edge_type& operator*() { return *operator->(); }
};

struct empty_vertex
{
};

struct empty_edge
{
};

template <typename V, typename E = empty_edge>
class directed_graph
{
public:
    using vertex_type = V;
    using edge_type = E;
    using self_type = directed_graph<vertex_type, edge_type>;

    using vertex_iterator = directed_graph_vertex_iterator<self_type>;
    using edge_iterator = directed_graph_edge_iterator<self_type>;

    using index_type = std::size_t;

public:
    template <typename... Args>
    vertex_iterator add_vertex(Args&&... args)
    {
        index_type index = allocate_vertex();
        m_vertices[index] = std::make_unique<vertex_wrapper>(std::forward<Args>(args)...);

        return vertex_iterator(this, index);
    }

    void remove_vertex(vertex_iterator vertex)
    {
        assert(vertex.m_graph == this);
        remove_vertex(vertex.m_index);
    }

    template <typename... Args>
    edge_iterator add_edge(vertex_iterator from, vertex_iterator to, Args&&... args)
    {
        assert(from.m_graph == this && to.m_graph == this);

        index_type index = allocate_edge();
        m_edges[index] =
            std::make_unique<edge_wrapper>(from.m_index, to.m_index, std::forward<Args>(args)...);

        m_vertices[from.m_index]->out.push_back(index);
        m_vertices[to.m_index]->in.push_back(index);

        return edge_iterator(this, index);
    }

    void remove_edge(edge_iterator edge)
    {
        assert(edge.m_graph == this);
        remove_edge(edge.m_index);
    }

    template <typename Functor>
    void bfs(vertex_iterator begin, Functor&& functor)
    {
        assert(begin.m_graph == this);

        std::vector<std::uint8_t> visited(m_vertices.size());

        std::queue<index_type> queue;
        queue.push(begin.m_index);

        while (!queue.empty())
        {
            index_type index = queue.front();
            queue.pop();

            visited[index] = 1;

            if (!functor(m_vertices[index]->vertex))
                break;

            for (index_type edge_index : m_vertices[index]->out)
            {
                index_type next_vertex_index = m_edges[edge_index]->to;
                if (visited[next_vertex_index] == 0)
                    queue.push(next_vertex_index);
            }
        }
    }

    template <typename Functor>
    void dfs(vertex_iterator begin, Functor&& functor)
    {
        assert(begin.m_graph == this);

        std::vector<std::uint8_t> visited(m_vertices.size());

        std::stack<index_type> stack;
        stack.push(begin.m_index);

        while (!stack.empty())
        {
            index_type index = stack.top();
            stack.pop();

            visited[index] = 1;

            if (!functor(m_vertices[index]->vertex))
                break;

            for (index_type edge_index : m_vertices[index]->out)
            {
                index_type next_vertex_index = m_edges[edge_index]->to;
                if (visited[next_vertex_index] == 0)
                    stack.push(next_vertex_index);
            }
        }
    }

private:
    friend class vertex_iterator;
    friend class edge_iterator;

    struct vertex_wrapper
    {
        template <typename... Args>
        vertex_wrapper(Args&&... args) : vertex(std::forward<Args>(args)...)
        {
        }

        std::vector<index_type> in;
        std::vector<index_type> out;
        vertex_type vertex;
    };

    struct edge_wrapper
    {
        template <typename... Args>
        edge_wrapper(index_type from, index_type to, Args&&... args)
            : from(from),
              to(to),
              edge(std::forward<Args>(args)...)
        {
        }

        index_type from;
        index_type to;
        edge_type edge;
    };

    void remove_vertex(index_type index)
    {
        vertex_wrapper& wrapper = *m_vertices[index];

        while (!wrapper.in.empty())
            remove_edge(wrapper.in.back());

        while (!wrapper.out.empty())
            remove_edge(wrapper.out.back());

        m_vertices[index] = nullptr;
        deallocate_vertex(index);
    }

    void remove_edge(index_type index)
    {
        edge_wrapper& wrapper = *m_edges[index];

        auto& from_out = m_vertices[wrapper.from]->out;
        from_out.erase(std::next(std::find(from_out.rbegin(), from_out.rend(), index)).base());

        auto& to_in = m_vertices[wrapper.to]->in;
        to_in.erase(std::next(std::find(to_in.rbegin(), to_in.rend(), index)).base());

        m_edges[index] = nullptr;
        deallocate_edge(index);
    }

    index_type allocate_vertex()
    {
        if (m_free_vertices.empty())
        {
            m_vertices.resize(m_vertices.size() + 1);
            return m_vertices.size() - 1;
        }
        else
        {
            index_type index = m_free_vertices.back();
            m_free_vertices.pop_back();
            return index;
        }
    }

    void deallocate_vertex(index_type index) { m_free_vertices.push_back(index); }

    index_type allocate_edge()
    {
        if (m_free_edges.empty())
        {
            m_edges.resize(m_edges.size() + 1);
            return m_edges.size() - 1;
        }
        else
        {
            index_type index = m_free_edges.back();
            m_free_edges.pop_back();
            return index;
        }
    }

    void deallocate_edge(index_type index) { m_free_edges.push_back(index); }

    std::vector<std::unique_ptr<vertex_wrapper>> m_vertices;
    std::vector<std::unique_ptr<edge_wrapper>> m_edges;

    std::vector<index_type> m_free_vertices;
    std::vector<index_type> m_free_edges;
};
} // namespace violet
#pragma once

#include "core/context.hpp"
#include "core/link.hpp"
#include "ecs/world.hpp"
#include <queue>
#include <stack>

namespace violet::core
{
class relation : public system_base
{
public:
    using link_type = link;

public:
    relation();

    virtual bool initialize(const dictionary& config) override;

    void link(ecs::entity child_entity, ecs::entity parent_entity);
    void unlink(ecs::entity entity);

    template <typename Functor>
    void each_bfs(ecs::entity entity, Functor&& functor, bool ignore_root = false)
    {
        auto& world = system<ecs::world>();

        std::queue<ecs::entity> bfs;
        if (ignore_root)
        {
            auto& node_link = world.component<link_type>(entity);
            for (auto child : node_link.children)
                bfs.push(child);
        }
        else
        {
            bfs.push(entity);
        }

        while (!bfs.empty())
        {
            ecs::entity node = bfs.front();
            bfs.pop();

            if constexpr (std::is_same_v<std::invoke_result_t<Functor, ecs::entity>, bool>)
            {
                if (!functor(node))
                    continue;
            }
            else
            {
                functor(node);
            }

            auto& node_link = world.component<link_type>(node);
            for (auto child : node_link.children)
                bfs.push(child);
        }
    }

    template <typename Functor>
    void each_dfs(ecs::entity entity, Functor&& functor, bool ignore_root = false)
    {
        auto& world = system<ecs::world>();

        std::stack<ecs::entity> dfs;
        if (ignore_root)
        {
            auto& node_link = world.component<link_type>(entity);
            for (auto child : node_link.children)
                dfs.push(child);
        }
        else
        {
            dfs.push(entity);
        }

        while (!dfs.empty())
        {
            ecs::entity node = dfs.top();
            dfs.pop();

            if constexpr (std::is_same_v<std::invoke_result_t<Functor, ecs::entity>, bool>)
            {
                if (!functor(node))
                    continue;
            }
            else
            {
                functor(node);
            }

            auto& node_link = world.component<link_type>(node);
            for (auto child : node_link.children)
                dfs.push(child);
        }
    }
};
} // namespace violet::core
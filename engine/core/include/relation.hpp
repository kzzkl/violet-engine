#pragma once

#include "context.hpp"
#include "link.hpp"
#include <queue>
#include <stack>

namespace ash::core
{
enum class event_unlink_flag
{
    UNLINK,
    BEFORE_LINK
};

struct event_link
{
    using functor = std::function<void(ash::ecs::entity, link&)>;
    using dispatcher = ash::core::sequence_dispatcher<functor>;
};

struct event_unlink
{
    using functor = std::function<void(ash::ecs::entity, link&, event_unlink_flag)>;
    using dispatcher = ash::core::sequence_dispatcher<functor>;
};

class relation : public system_base
{
public:
    using link_type = link;

public:
    relation();

    virtual bool initialize(const dictionary& config) override;

    void link(ecs::entity child_entity, ecs::entity parent_entity);
    void unlink(ecs::entity entity, bool before_link = false);

    template <typename Functor>
    void each_bfs(ecs::entity entity, Functor&& functor)
    {
        auto& world = system<ecs::world>();

        std::queue<ecs::entity> bfs;
        bfs.push(entity);

        while (!bfs.empty())
        {
            ecs::entity node = bfs.front();
            bfs.pop();

            if (!world.has_component<link_type>(node))
                continue;

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
    void each_dfs(ecs::entity entity, Functor&& functor)
    {
        auto& world = system<ecs::world>();

        std::stack<ecs::entity> dfs;
        dfs.push(entity);

        while (!dfs.empty())
        {
            ecs::entity node = dfs.top();
            dfs.pop();

            if (!world.has_component<link_type>(node))
                continue;

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

private:
};
} // namespace ash::core
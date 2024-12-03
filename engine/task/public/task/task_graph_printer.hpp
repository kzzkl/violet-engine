#pragma once

#include "task/task_graph.hpp"
#include <iostream>
#include <stack>
#include <string>
#include <unordered_map>

namespace violet
{
class task_graph_printer
{
public:
    static void print(const task_graph& graph)
    {
        std::cout << "flowchart LR\n";

        const auto& roots = graph.get_root_tasks();

        std::stack<task_wrapper*> stack;

        int id = 0;

        struct task_info
        {
            int id;
            std::string name;
            bool is_group_begin;
            bool is_group_end;

            std::string get_node_name() const
            {
                return std::to_string(id) + '[' + name + ']';
            }
        };
        std::unordered_map<task_wrapper*, task_info> task_infos;

        auto get_task_info = [&](task_wrapper* task) -> task_info
        {
            auto iter = task_infos.find(task);
            if (iter != task_infos.end())
            {
                return iter->second;
            }

            task_info info = {};

            if (task->get_name().ends_with(task_group::group_begin_suffix))
            {
                std::string_view group_name = task->get_name().substr(
                    0,
                    task->get_name().size() - task_group::group_begin_suffix.size());

                info.id = ++id;
                info.name = std::string(group_name.data(), group_name.size());
                info.is_group_begin = true;
            }
            else if (task->get_name().ends_with(task_group::group_end_suffix))
            {
                std::string_view group_name = task->get_name().substr(
                    0,
                    task->get_name().size() - task_group::group_end_suffix.size());

                for (auto& [t, i] : task_infos)
                {
                    if (i.is_group_begin && i.name == group_name)
                    {
                        info.id = i.id;
                        info.name = i.name;
                        info.is_group_end = true;
                        break;
                    }
                }
            }
            else
            {
                info.id = ++id;
                info.name = task->get_name();
            }

            task_infos[task] = info;
            return info;
        };

        for (task_wrapper* root : roots)
        {
            stack.push(root);

            if (root->successors.empty())
            {
                std::cout << get_task_info(root).get_node_name() << '\n';
            }
        }

        std::unordered_map<task_wrapper*, bool> visited;
        while (!stack.empty())
        {
            task_wrapper* node = stack.top();
            stack.pop();

            if (visited[node])
            {
                continue;
            }

            visited[node] = true;

            task_info info = get_task_info(node);

            if (info.is_group_begin)
            {
                std::cout << "subgraph " << info.get_node_name() << '\n';

                for (task_wrapper* successor : node->successors)
                {
                    if (successor->successors.empty())
                    {
                        std::cout << get_task_info(successor).get_node_name() << '\n';
                    }
                    stack.push(successor);
                }
            }
            else if (info.is_group_end)
            {
                std::cout << "end" << '\n';

                for (task_wrapper* successor : node->successors)
                {
                    std::cout << info.get_node_name() << "-->"
                              << get_task_info(successor).get_node_name() << '\n';
                    stack.push(successor);
                }
            }
            else
            {
                for (task_wrapper* successor : node->successors)
                {
                    task_info successor_info = get_task_info(successor);
                    if (successor_info.is_group_end)
                    {
                        std::cout << info.get_node_name() << '\n';
                    }
                    else
                    {
                        std::cout << info.get_node_name() << "-->" << successor_info.get_node_name()
                                  << '\n';
                    }
                    stack.push(successor);
                }
            }
        }

        for (auto& [task, info] : task_infos)
        {
            if (info.is_group_begin || info.is_group_end)
            {
                continue;
            }

            if (task->get_options() & TASK_OPTION_MAIN_THREAD)
            {
                std::cout << "style " << info.id << " fill:#668" << '\n';
            }
        }
    }
};
} // namespace violet
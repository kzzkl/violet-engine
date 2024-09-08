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

        auto& roots = graph.get_root_tasks();

        std::stack<task_wrapper*> stack;

        int id = 0;
        std::unordered_map<task_wrapper*, bool> visited;
        std::unordered_map<task_wrapper*, int> node_id;

        auto get_node_name = [&](task_wrapper* node) -> std::string
        {
            if (node_id[node] == 0)
            {
                node_id[node] = ++id;
            }

            return std::to_string(node_id[node]) + '[' + node->get_name().data() + ']';
        };

        for (task_wrapper* root : roots)
        {
            stack.push(root);

            if (root->successors.empty())
            {
                std::cout << get_node_name(root) << '\n';
            }
        }

        while (!stack.empty())
        {
            task_wrapper* node = stack.top();
            stack.pop();

            if (visited[node])
            {
                continue;
            }

            visited[node] = true;

            for (task_wrapper* successor : node->successors)
            {
                std::cout << get_node_name(node) << "-->" << get_node_name(successor) << '\n';
                stack.push(successor);
            }
        }

        for (auto& [task, id] : node_id)
        {
            if (task->get_options() & TASK_OPTION_MAIN_THREAD)
            {
                std::cout << "style " << id << " fill:#668" << '\n';
            }
        }
    }
};
} // namespace violet
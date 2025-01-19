#pragma once

#include "common/type_index.hpp"
#include "graphics/render_graph/rdg_node.hpp"
#include "graphics/render_graph/rdg_reference.hpp"
#include <cassert>
#include <memory>

namespace violet
{
class rdg_allocator
{
public:
    rdg_allocator() = default;

    template <typename T>
    T* allocate_resource()
    {
        return allocate<T>(
            [this]()
            {
                return std::make_unique<T>();
            });
    }

    template <typename T>
    T* allocate_pass()
    {
        return allocate<T>(
            [this]()
            {
                return std::make_unique<T>(this);
            });
    }

    rdg_reference* allocate_reference()
    {
        return allocate<rdg_reference>(
            []()
            {
                return std::make_unique<rdg_reference>();
            });
    }

    void tick()
    {
        for (auto& pool : m_node_pools)
        {
            pool.count = 0;
        }

        m_edge_pool.count = 0;
    }

private:
    struct object_index : public type_index<object_index, std::size_t>
    {
    };

    template <typename T, typename CreateFunctor>
    T* allocate(CreateFunctor&& create)
    {
        T* result = nullptr;

        if constexpr (std::is_base_of_v<rdg_node, T>)
        {
            std::size_t index = object_index::value<T>();
            if (index >= m_node_pools.size())
            {
                m_node_pools.resize(index + 1);
            }

            auto& pool = m_node_pools[index];

            if (pool.count == pool.objects.size())
            {
                pool.objects.push_back(create());
            }

            result = static_cast<T*>(pool.objects[pool.count++].get());
            result->reset();
        }

        if constexpr (std::is_same_v<T, rdg_reference>)
        {
            if (m_edge_pool.count == m_edge_pool.objects.size())
            {
                m_edge_pool.objects.push_back(create());
            }

            result = m_edge_pool.objects[m_edge_pool.count++].get();
        }

        return result;
    }

    template <typename ObjectType>
    struct object_pool
    {
        std::size_t count{0};
        std::vector<std::unique_ptr<ObjectType>> objects;
    };

    std::vector<object_pool<rdg_node>> m_node_pools;
    object_pool<rdg_reference> m_edge_pool;
};
} // namespace violet
#include "archetype.hpp"
#include "assert.hpp"
#include <algorithm>

namespace ash::ecs
{
archetype_layout::archetype_layout(std::size_t capacity) : m_capacity(capacity)
{
}

void archetype_layout::rebuild()
{
    m_entity_per_chunk = 0;

    info_list list;
    for (auto& [type, info] : m_layout)
        list.push_back(std::make_pair(type, info));

    if (list.empty())
        return;

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        return a.second.data->align == b.second.data->align
                   ? a.first < b.first
                   : a.second.data->align > b.second.data->align;
    });

    std::size_t entitySize = 0;
    for (const auto& [type, info] : list)
        entitySize += info.data->size;

    m_entity_per_chunk = m_capacity / entitySize;

    std::size_t offset = 0;
    for (auto& [type, info] : list)
    {
        info.offset = offset;
        m_layout[type] = info;

        offset += info.data->size * m_entity_per_chunk;
    }
}

archetype::archetype(const archetype_layout& layout)
    : m_layout(layout),
      m_storage(layout.get_entity_per_chunk())
{
}

void archetype::add()
{
    storage::handle handle = m_storage.push_back();
    construct(handle);
}

void archetype::remove(std::size_t index)
{
    auto handle = m_storage.begin() + index;
    auto back = m_storage.end() - 1;

    if (handle != back)
        swap(handle, back);

    destruct(back);

    m_storage.pop_back();
}

void archetype::move(std::size_t index, archetype& target)
{
    auto target_handle = target.m_storage.push_back();
    auto source_handle = m_storage.begin() + index;

    auto& target_layout = target.m_layout;
    auto& source_layout = m_layout;

    for (auto& [type, component] : source_layout)
    {
        if (target_layout.find(type) != target_layout.end())
        {
            component.data->move_construct(
                source_handle.get_component(component.offset, component.data->size),
                target_handle.get_component(component.offset, component.data->size));
        }
    }

    for (auto [type, component] : target_layout)
    {
        if (source_layout.find(type) == source_layout.end())
        {
            component.data->construct(
                target_handle.get_component(component.offset, component.data->size));
        }
    }

    remove(index);
}

void archetype::construct(storage::handle where)
{
    for (auto& [type, component] : m_layout)
    {
        component.data->construct(where.get_component(component.offset, component.data->size));
    }
}

void archetype::move_construct(storage::handle source, storage::handle target)
{
    for (auto& [type, component] : m_layout)
    {
        component.data->move_construct(
            source.get_component(component.offset, component.data->size),
            target.get_component(component.offset, component.data->size));
    }
}

void archetype::destruct(storage::handle where)
{
    for (auto& [type, component] : m_layout)
    {
        component.data->destruct(where.get_component(component.offset, component.data->size));
    }
}

void archetype::swap(storage::handle a, storage::handle b)
{
    for (auto& [type, component] : m_layout)
    {
        component.data->swap(
            a.get_component(component.offset, component.data->size),
            b.get_component(component.offset, component.data->size));
    }
}

void* archetype::get_component(std::size_t index, component_id type)
{
    auto& layout = m_layout[type];
    auto handle = m_storage.begin() + index;
    return handle.get_component(layout.offset, layout.data->size);
}
} // namespace ash::ecs
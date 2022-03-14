#include "archetype.hpp"
#include "assert.hpp"
#include <algorithm>

namespace ash::ecs
{
archetype_layout::archetype_layout(std::size_t capacity) : m_capacity(capacity)
{
}

void archetype_layout::rebuild(info_list& list)
{
    if (list.empty())
    {
        m_mask.reset();
        m_entity_per_chunk = 0;
        m_layout.clear();
        return;
    }

    std::sort(list.begin(), list.end(), [](const auto& a, const auto& b) {
        return a.second.layout.align == b.second.layout.align
                   ? a.first < b.first
                   : a.second.layout.align > b.second.layout.align;
    });

    auto duplicate =
        std::unique(list.begin(), list.end(), [](const auto& a, const auto& b) -> bool {
            return a.first == b.first;
        });
    list.erase(duplicate, list.end());

    std::size_t entitySize = 0;
    for (const auto& [type, info] : list)
        entitySize += info.layout.size;

    m_entity_per_chunk = m_capacity / entitySize;
    m_layout.clear();

    std::size_t offset = 0;
    for (auto& [type, info] : list)
    {
        info.layout.offset = offset;
        m_layout[type] = info;

        offset += info.layout.size * m_entity_per_chunk;
    }
}

archetype_layout::info_list archetype_layout::get_info_list() const
{
    info_list result;
    for (auto& [type, info] : m_layout)
        result.emplace_back(std::make_pair(type, info));

    return result;
}

archetype::archetype(const archetype_layout& layout)
    : m_layout(layout),
      m_storage(layout.get_entity_per_chunk())
{
}

std::size_t archetype::add()
{
    storage::handle handle = m_storage.push_back();
    construct(handle);

    return handle - m_storage.begin();
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
    component_mask mask = m_layout.get_mask() & target.m_layout.get_mask();

    auto target_handle = target.m_storage.push_back();
    auto source_handle = m_storage.begin() + index;

    for (auto& [type, component] : m_layout)
    {
        if (mask.test(type))
        {
            component.functor.moveConstruct(
                source_handle.get_component(component.layout.offset, component.layout.size),
                target_handle.get_component(component.layout.offset, component.layout.size));
        }
        else
        {
            component.functor.destruct(
                source_handle.get_component(component.layout.offset, component.layout.size));
        }
    }

    for (auto& [type, component] : target.m_layout)
    {
        if (!mask.test(type))
        {
            component.functor.construct(
                target_handle.get_component(component.layout.offset, component.layout.size));
        }
    }
}

void archetype::construct(storage::handle where)
{
    for (auto& [type, component] : m_layout)
    {
        component.functor.construct(
            where.get_component(component.layout.offset, component.layout.size));
    }
}

void archetype::move_construct(storage::handle source, storage::handle target)
{
    for (auto& [type, component] : m_layout)
    {
        component.functor.moveConstruct(
            source.get_component(component.layout.offset, component.layout.size),
            target.get_component(component.layout.offset, component.layout.size));
    }
}

void archetype::destruct(storage::handle where)
{
    for (auto& [type, component] : m_layout)
    {
        component.functor.destruct(
            where.get_component(component.layout.offset, component.layout.size));
    }
}

void archetype::swap(storage::handle a, storage::handle b)
{
    for (auto& [type, component] : m_layout)
    {
        component.functor.swap(
            a.get_component(component.layout.offset, component.layout.size),
            b.get_component(component.layout.offset, component.layout.size));
    }
}

void* archetype::get_component(std::size_t index, component_index type)
{
    auto& layout = m_layout[type].layout;
    auto handle = m_storage.begin() + index;
    return handle.get_component(layout.offset, layout.size);
}
} // namespace ash::ecs
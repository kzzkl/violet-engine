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

void redirector::map(entity entity, std::size_t index)
{
    ASH_ASSERT(has_entity(entity) == false);

    if (index >= m_entities.size())
        m_entities.resize(index + 1, INVALID_ENTITY);

    m_redirector[entity] = index;
    m_entities[index] = entity;
}

void redirector::unmap(entity entity)
{
    ASH_ASSERT(has_entity(entity) == true);

    auto iter = m_redirector.find(entity);
    m_entities[iter->second] = INVALID_ENTITY;
    m_redirector.erase(iter);
}

entity redirector::get_enitiy(std::size_t index) const
{
    return m_entities[index];
}

std::size_t redirector::get_index(entity entity) const
{
    ASH_ASSERT(has_entity(entity));
    return m_redirector.find(entity)->second;
}

bool redirector::has_entity(entity entity) const
{
    return m_redirector.find(entity) != m_redirector.cend();
}

std::size_t redirector::size() const
{
    return m_redirector.size();
}

archetype::archetype(const archetype_layout& layout)
    : m_layout(layout),
      m_storage(layout.get_entity_per_chunk())
{
}

archetype::raw_handle archetype::add(entity entity)
{
    storage::handle handle = m_storage.insert_end();
    construct(handle);

    std::size_t index = handle - m_storage.begin();
    m_redirector.map(entity, index);

    return raw_handle{this, index};
}

void archetype::remove(entity entity)
{
    std::size_t index = m_redirector.get_index(entity);

    auto handle = m_storage.begin() + index;
    auto back = m_storage.end() - 1;

    if (handle != back)
        swap(handle, back);

    destruct(back);

    m_storage.erase_end();
    m_redirector.unmap(entity);
}

void archetype::move(entity entity, archetype& target)
{
    component_mask mask = m_layout.get_mask() & target.m_layout.get_mask();

    auto targethandle = target.m_storage.insert_end();
    auto sourcehandle = m_storage.begin() + m_redirector.get_index(entity);

    target.m_redirector.map(entity, targethandle - target.m_storage.begin());

    for (auto& [type, component] : m_layout)
    {
        if (mask.test(type))
        {
            component.functor.moveConstruct(
                sourcehandle.get_component(component.layout.offset, component.layout.size),
                targethandle.get_component(component.layout.offset, component.layout.size));
        }
        else
        {
            component.functor.destruct(
                sourcehandle.get_component(component.layout.offset, component.layout.size));
        }
    }

    for (auto& [type, component] : target.m_layout)
    {
        if (!mask.test(type))
        {
            component.functor.construct(
                targethandle.get_component(component.layout.offset, component.layout.size));
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
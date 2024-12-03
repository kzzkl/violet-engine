#include "ecs/world.hpp"
#include "archetype_chunk.hpp"

namespace violet
{
world::world()
{
    m_main_thread_id = std::this_thread::get_id();

    m_archetype_chunk_allocator = std::make_unique<archetype_chunk_allocator>();
    m_entities.resize(1);

    register_component<entity>();
}

world::~world()
{
    for (auto& [_, archetype] : m_archetypes)
    {
        archetype->clear();
    }
}

entity world::create()
{
    entity result;
    result.type = ENTITY_NORMAL;

    if (m_free_entity.empty())
    {
        result.id = m_entities.size();
        m_entities.emplace_back();
    }
    else
    {
        result.id = m_free_entity.front();
        m_free_entity.pop();
        result.version = m_entities[result.id].version;
    }

    add_component<entity>(result);

    return result;
}

void world::destroy(entity e)
{
    assert(is_main_thread());
    assert(is_valid(e));
    destroy_entity(e.id);
}

bool world::is_valid(entity e) const
{
    return e.id < m_entities.size() && e.type != ENTITY_NULL &&
           m_entities[e.id].version == e.version;
}

void world::execute(std::span<world_command*> commands)
{
    assert(is_main_thread());

    struct entity_state
    {
        bool destroyed{false};
        component_mask mask;
        std::vector<component_id> components;
        std::vector<void*> component_data;
    };
    std::unordered_map<entity_id, entity_state> normal_entity_states;
    std::vector<entity_state> temp_entity_states;

    auto get_state = [&, this](entity e) -> entity_state&
    {
        if (e.type == ENTITY_TEMPORARY)
        {
            return temp_entity_states[e.id];
        }

        auto iter = normal_entity_states.find(e.id);
        if (iter == normal_entity_states.end())
        {
            archetype* archetype = m_entities[e.id].archetype;

            entity_state state = {};
            state.destroyed = false;
            state.mask = archetype->get_mask();
            state.components = archetype->get_component_ids();
            state.component_data.resize(state.components.size());

            normal_entity_states[e.id] = state;

            return normal_entity_states[e.id];
        }

        return iter->second;
    };

    auto change_entity_archetype = [&, this](entity_id id, const entity_state& state)
    {
        archetype* new_archetype = nullptr;

        auto iter = m_archetypes.find(state.mask);
        if (iter == m_archetypes.cend())
        {
            std::vector<component_id> components;
            for (component_id component_id : state.components)
            {
                if (state.mask.test(component_id))
                {
                    components.push_back(component_id);
                }
            }

            new_archetype = create_archetype(components);
        }
        else
        {
            new_archetype = iter->second.get();
        }

        auto& info = m_entities[id];

        archetype* old_archetype = info.archetype;
        if (new_archetype == old_archetype)
        {
            return;
        }

        std::size_t new_archetype_index =
            old_archetype == nullptr ?
                new_archetype->add(m_world_version) :
                old_archetype->move(info.archetype_index, *new_archetype, m_world_version);

        move_entity(id, new_archetype, new_archetype_index);

        for (std::size_t i = 0; i < state.components.size(); ++i)
        {
            if (state.mask.test(state.components[i]) && state.component_data[i] != nullptr)
            {
                void* src = state.component_data[i];
                void* dst = new_archetype->get_component_pointer(
                    state.components[i],
                    info.archetype_index,
                    m_world_version);

                m_components[state.components[i]].builder->move_assignment(src, dst);
            }
        }
    };

    for (world_command* command : commands)
    {
        if (command->get_commands().empty())
        {
            continue;
        }

        temp_entity_states.resize(command->get_temp_entity_count());

        for (const auto& cmd : command->get_commands())
        {
            switch (cmd.type)
            {
            case world_command::COMMAND_CREATE: {
                entity_state& state = get_state(cmd.e);
                state.mask.set(component_index::value<entity>());
                state.components.push_back(component_index::value<entity>());
                state.component_data.push_back(nullptr);
                break;
            }
            case world_command::COMMAND_DESTROY: {
                entity_state& state = get_state(cmd.e);
                state.destroyed = true;
                break;
            }
            case world_command::COMMAND_ADD_COMPONENT: {
                entity_state& state = get_state(cmd.e);
                state.mask.set(cmd.component);
                state.components.push_back(cmd.component);
                state.component_data.push_back(cmd.component_data);
                break;
            }
            case world_command::COMMAND_REMOVE_COMPONENT: {
                entity_state& state = get_state(cmd.e);
                state.mask.reset(cmd.component);
                break;
            }
            default:
                break;
            }
        }

        for (auto& state : temp_entity_states)
        {
            if (state.destroyed)
            {
                continue;
            }

            entity new_e = create();
            change_entity_archetype(new_e.id, state);
        }

        temp_entity_states.clear();
    }

    for (auto& [id, state] : normal_entity_states)
    {
        if (state.destroyed)
        {
            destroy_entity(id);
        }
        else
        {
            change_entity_archetype(id, state);
        }
    }

    for (auto& cmd : commands)
    {
        cmd->reset();
    }
}

void world::destroy_entity(entity_id id)
{
    entity_info& info = m_entities[id];
    if (info.archetype != nullptr)
    {
        info.archetype->remove(info.archetype_index);
        move_entity(id, nullptr, 0);
    }

    info.archetype = nullptr;
    info.archetype_index = 0;
    ++info.version;

    m_free_entity.push(id);
}

void world::move_entity(entity_id id, archetype* new_archetype, std::size_t new_archetype_index)
{
    entity_info& info = m_entities[id];
    if (info.archetype != nullptr && info.archetype->get_entity_count() > info.archetype_index)
    {
        auto [e] =
            info.archetype->get_components<const entity>(info.archetype_index, m_world_version);

        m_entities[e->id].archetype_index = info.archetype_index;
    }

    m_entities[id].archetype = new_archetype;
    m_entities[id].archetype_index = new_archetype_index;
}

archetype* world::create_archetype(std::span<const component_id> components)
{
    archetype_layout layout;
    layout.reserve(components.size());

    for (component_id component_id : components)
    {
        layout.push_back({component_id, m_components[component_id].builder.get()});
    }

    auto result = std::make_unique<archetype>(layout, m_archetype_chunk_allocator.get());
    return (m_archetypes[result->get_mask()] = std::move(result)).get();
}
} // namespace violet
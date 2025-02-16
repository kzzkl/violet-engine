#pragma once

#include "common/allocator.hpp"
#include "graphics/resources/buffer.hpp"

namespace violet
{
template <typename T>
class render_object_container
{
public:
    using cpu_type = T;
    using gpu_type = T::gpu_type;

    render_object_container()
    {
        m_object_buffer = std::make_unique<structured_buffer>(
            64 * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
    }

    render_id add()
    {
        render_id id = m_id_allocator.allocate();

        if (m_id_to_index.size() <= id)
        {
            m_id_to_index.resize(id + 1);
            m_objects.resize(id + 1);
        }

        m_id_to_index[id] = m_index_to_id.size();
        m_index_to_id.push_back(id);

        m_objects[id].states = OBJECT_STAGE_VALID;
        mark_dirty(id);

        return id;
    }

    render_id remove(render_id id)
    {
        render_id last_id = m_index_to_id.back();
        if (last_id != id)
        {
            std::size_t index = m_id_to_index[id];

            m_index_to_id[index] = last_id;
            m_id_to_index[last_id] = index;
        }

        m_index_to_id.pop_back();
        m_id_allocator.free(id);

        m_objects[id].states = 0;

        return last_id;
    }

    T& operator[](render_id id) noexcept
    {
        return m_objects[id].data;
    }

    void mark_dirty(render_id id)
    {
        auto& object = m_objects[id];

        assert(object.is_valid());

        if ((object.states & OBJECT_STAGE_DIRTY) == 0)
        {
            object.states |= OBJECT_STAGE_DIRTY;
            m_dirty_objects.push_back(id);
        }
    }

    render_id get_id(std::size_t index) const noexcept
    {
        return m_index_to_id[index];
    }

    std::size_t get_index(render_id id) const noexcept
    {
        return m_id_to_index[id];
    }

    std::size_t get_size() const noexcept
    {
        return m_index_to_id.size();
    }

    std::size_t get_capacity() const noexcept
    {
        return m_object_buffer->get_size() / sizeof(gpu_type);
    }

    structured_buffer* get_buffer() const noexcept
    {
        return m_object_buffer.get();
    }

    template <typename Functor>
    void update(Functor functor)
    {
        if (m_dirty_objects.empty())
        {
            return;
        }

        if (get_size() > get_capacity())
        {
            reserve(get_size());

            for (std::size_t i = 0; i < m_index_to_id.size(); ++i)
            {
                render_id id = m_index_to_id[i];

                functor(m_objects[id].data, id, i);
                m_objects[id].states &= ~OBJECT_STAGE_DIRTY;
            }
        }
        else
        {
            for (render_id id : m_dirty_objects)
            {
                functor(m_objects[id].data, id, m_id_to_index[id]);
                m_objects[id].states &= ~OBJECT_STAGE_DIRTY;
            }
        }

        m_dirty_objects.clear();
    }

private:
    enum object_state
    {
        OBJECT_STAGE_VALID = 1 << 0,
        OBJECT_STAGE_DIRTY = 1 << 1,
    };
    using object_states = std::uint32_t;

    struct object_wrapper
    {
        cpu_type data;

        object_states states;

        bool is_valid() const noexcept
        {
            return states & OBJECT_STAGE_VALID;
        }
    };

    void reserve(std::size_t object_count)
    {
        std::size_t capacity = get_capacity();
        while (capacity < object_count)
        {
            capacity += capacity / 2;
        }

        m_object_buffer = std::make_unique<structured_buffer>(
            capacity * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
    }

    std::vector<object_wrapper> m_objects;
    std::vector<render_id> m_dirty_objects;

    std::vector<std::size_t> m_id_to_index;
    std::vector<render_id> m_index_to_id;

    index_allocator<render_id> m_id_allocator;

    std::unique_ptr<structured_buffer> m_object_buffer;
};
} // namespace violet
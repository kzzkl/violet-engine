#pragma once

#include "common/allocator.hpp"
#include "graphics/resources/buffer.hpp"

namespace violet
{
template <typename T>
class gpu_dense_array
{
public:
    using cpu_type = T;
    using gpu_type = T::gpu_type;

    gpu_dense_array()
    {
        m_object_buffer = std::make_unique<structured_buffer>(
            64 * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
    }

    render_id add()
    {
        render_id id = m_allocator.allocate();

        if (m_id_to_index.size() <= id)
        {
            m_id_to_index.resize(id + 1);
            m_objects.resize(id + 1);
        }

        m_id_to_index[id] = m_index_to_id.size();
        m_index_to_id.push_back(id);

        m_objects[id].valid = true;
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

            mark_dirty(last_id);
        }

        m_index_to_id.pop_back();
        m_allocator.free(id);

        m_objects[id].cpu_data = {};
        m_objects[id].valid = false;
        m_objects[id].dirty = false;

        return last_id;
    }

    T& operator[](render_id id) noexcept
    {
        return m_objects[id].cpu_data;
    }

    const T& operator[](render_id id) const noexcept
    {
        return m_objects[id].cpu_data;
    }

    void mark_dirty(render_id id)
    {
        auto& object = m_objects[id];

        assert(object.valid);

        if (!object.dirty)
        {
            object.dirty = true;
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

    template <typename GetDataFunctor, typename UploadFunctor>
    bool update(GetDataFunctor&& get_data, UploadFunctor&& upload, bool update_all = false)
    {
        if (m_dirty_objects.empty())
        {
            return false;
        }

        bool need_resize = false;

        if (get_size() > get_capacity())
        {
            reserve(get_size());
            need_resize = true;
        }

        if (update_all || need_resize)
        {
            std::vector<gpu_type> gpu_datas;

            for (render_id id : m_index_to_id)
            {
                gpu_datas.push_back(get_data(m_objects[id].cpu_data));
                m_objects[id].dirty = false;
            }

            upload(
                m_object_buffer->get_rhi(),
                gpu_datas.data(),
                gpu_datas.size() * sizeof(gpu_type),
                0);
        }
        else
        {
            for (render_id id : m_dirty_objects)
            {
                if (!m_objects[id].valid)
                {
                    continue;
                }

                gpu_type gpu_data = get_data(m_objects[id].cpu_data);

                upload(
                    m_object_buffer->get_rhi(),
                    &gpu_data,
                    sizeof(gpu_type),
                    m_id_to_index[id] * sizeof(gpu_type));

                m_objects[id].dirty = false;
            }
        }

        m_dirty_objects.clear();

        return need_resize;
    }

private:
    struct wrapper
    {
        cpu_type cpu_data;

        bool dirty;
        bool valid;
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

    std::vector<wrapper> m_objects;
    std::vector<render_id> m_dirty_objects;

    std::vector<std::size_t> m_id_to_index;
    std::vector<render_id> m_index_to_id;

    index_allocator<render_id> m_allocator;

    std::unique_ptr<structured_buffer> m_object_buffer;
};

template <typename T>
class gpu_sparse_array
{
public:
    using cpu_type = T;
    using gpu_type = T::gpu_type;

    gpu_sparse_array()
    {
        m_object_buffer = std::make_unique<structured_buffer>(
            64 * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
    }

    render_id add()
    {
        render_id id = m_allocator.allocate();

        if (m_objects.size() <= id)
        {
            m_objects.resize(id + 1);
        }

        m_objects[id].valid = true;
        mark_dirty(id);

        return id;
    }

    void remove(render_id id)
    {
        m_allocator.free(id);

        m_objects[id].valid = false;
        m_objects[id].dirty = false;
    }

    T& operator[](render_id id) noexcept
    {
        return m_objects[id].cpu_data;
    }

    const T& operator[](render_id id) const noexcept
    {
        return m_objects[id].cpu_data;
    }

    void mark_dirty(render_id id)
    {
        auto& object = m_objects[id];

        assert(object.valid);

        if (!object.dirty)
        {
            object.dirty = true;
            m_dirty_objects.push_back(id);
        }
    }

    std::size_t get_size() const noexcept
    {
        return m_allocator.get_size();
    }

    std::size_t get_capacity() const noexcept
    {
        return m_object_buffer->get_size() / sizeof(gpu_type);
    }

    structured_buffer* get_buffer() const noexcept
    {
        return m_object_buffer.get();
    }

    template <typename GetDataFunctor, typename UploadFunctor>
    bool update(GetDataFunctor&& get_data, UploadFunctor&& upload, bool update_all = false)
    {
        if (m_dirty_objects.empty())
        {
            return false;
        }

        bool need_resize = false;

        if (get_size() > get_capacity())
        {
            reserve(get_size());
            need_resize = true;
        }

        if (update_all || need_resize)
        {
            std::vector<gpu_type> gpu_datas;

            for (auto& wrapper : m_objects)
            {
                gpu_datas.push_back(get_data(wrapper.cpu_data));
                wrapper.dirty = false;
            }

            upload(
                m_object_buffer->get_rhi(),
                gpu_datas.data(),
                gpu_datas.size() * sizeof(gpu_type),
                0);
        }
        else
        {
            for (render_id id : m_dirty_objects)
            {
                if (!m_objects[id].valid)
                {
                    continue;
                }

                gpu_type gpu_data = get_data(m_objects[id].cpu_data);

                upload(
                    m_object_buffer->get_rhi(),
                    &gpu_data,
                    sizeof(gpu_type),
                    id * sizeof(gpu_type));

                m_objects[id].dirty = false;
            }
        }

        m_dirty_objects.clear();

        return need_resize;
    }

private:
    struct wrapper
    {
        cpu_type cpu_data;

        bool dirty;
        bool valid;
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

    std::vector<wrapper> m_objects;
    std::vector<render_id> m_dirty_objects;

    index_allocator<render_id> m_allocator;

    std::unique_ptr<structured_buffer> m_object_buffer;
};
} // namespace violet
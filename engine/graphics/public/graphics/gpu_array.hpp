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

    gpu_dense_array(std::uint32_t id_offset = 0, std::uint32_t index_offset = 0)
        : m_index_to_id(index_offset, INVALID_RENDER_ID),
          m_allocator(id_offset)
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

        m_id_to_index[id] = get_size();
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
            std::uint32_t index = m_id_to_index[id];

            m_index_to_id[index] = last_id;
            m_id_to_index[last_id] = index;

            mark_dirty(last_id);
        }

        m_index_to_id.pop_back();

        m_objects[id].cpu_data = {};
        m_objects[id].valid = false;
        m_objects[id].dirty = false;

        m_allocator.free(id);

        return last_id;
    }

    T& operator[](render_id id)
    {
        return m_objects[id].cpu_data;
    }

    const T& operator[](render_id id) const
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
                if (!m_objects[id].valid || !m_objects[id].dirty)
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

    render_id get_id(std::uint32_t index) const
    {
        return m_index_to_id[index];
    }

    std::uint32_t get_index(render_id id) const
    {
        return m_id_to_index[id];
    }

    std::uint32_t get_size() const noexcept
    {
        return static_cast<std::uint32_t>(m_index_to_id.size());
    }

    std::uint32_t get_capacity() const noexcept
    {
        return static_cast<std::uint32_t>(m_object_buffer->get_size() / sizeof(gpu_type));
    }

    structured_buffer* get_buffer() const noexcept
    {
        return m_object_buffer.get();
    }

    template <typename Functor>
    void each(Functor&& functor)
    {
        for (render_id id : m_index_to_id)
        {
            functor(id, m_objects[id].cpu_data);
        }
    }

    template <typename Functor>
    void each(Functor&& functor) const
    {
        for (render_id id : m_index_to_id)
        {
            functor(id, m_objects[id].cpu_data);
        }
    }

    void set_name(std::string_view name)
    {
        m_name = name;
        m_object_buffer->get_rhi()->set_name(m_name.c_str());
    }

private:
    struct wrapper
    {
        cpu_type cpu_data;

        bool dirty;
        bool valid;
    };

    void reserve(std::uint32_t object_count)
    {
        std::uint32_t capacity = get_capacity();
        while (capacity < object_count)
        {
            capacity += capacity / 2;
        }

        m_object_buffer = std::make_unique<structured_buffer>(
            capacity * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

        if (!m_name.empty())
        {
            m_object_buffer->get_rhi()->set_name(m_name.c_str());
        }
    }

    std::vector<wrapper> m_objects;
    std::vector<render_id> m_dirty_objects;

    std::vector<std::uint32_t> m_id_to_index;
    std::vector<render_id> m_index_to_id;

    index_allocator m_allocator;

    std::unique_ptr<structured_buffer> m_object_buffer;

    std::string m_name;
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

    T& operator[](render_id id)
    {
        return m_objects[id].cpu_data;
    }

    const T& operator[](render_id id) const
    {
        return m_objects[id].cpu_data;
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
        m_objects[id].valid = false;
        m_objects[id].dirty = false;
        m_allocator.free(id);
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

    template <typename Functor>
    void each(Functor&& functor)
    {
        for (render_id id = 0; id < m_objects.size(); ++id)
        {
            if (m_objects[id].valid)
            {
                functor(id, m_objects[id].cpu_data);
            }
        }
    }

    template <typename Functor>
    void each(Functor&& functor) const
    {
        for (render_id id = 0; id < m_objects.size(); ++id)
        {
            if (m_objects[id].valid)
            {
                functor(id, m_objects[id].cpu_data);
            }
        }
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
                if (!m_objects[id].valid || !m_objects[id].dirty)
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

    std::uint32_t get_size() const noexcept
    {
        return static_cast<std::uint32_t>(m_allocator.get_size());
    }

    std::uint32_t get_capacity() const noexcept
    {
        return static_cast<std::uint32_t>(m_object_buffer->get_size() / sizeof(gpu_type));
    }

    structured_buffer* get_buffer() const noexcept
    {
        return m_object_buffer.get();
    }

    void set_name(std::string_view name)
    {
        m_name = name;
        m_object_buffer->get_rhi()->set_name(m_name.c_str());
    }

private:
    struct wrapper
    {
        cpu_type cpu_data;

        bool dirty;
        bool valid;
    };

    void reserve(std::uint32_t object_count)
    {
        std::uint32_t capacity = get_capacity();
        while (capacity < object_count)
        {
            capacity += capacity / 2;
        }

        m_object_buffer = std::make_unique<structured_buffer>(
            capacity * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

        if (!m_name.empty())
        {
            m_object_buffer->get_rhi()->set_name(m_name.c_str());
        }
    }

    std::vector<wrapper> m_objects;
    std::vector<render_id> m_dirty_objects;

    index_allocator m_allocator;
    std::unique_ptr<structured_buffer> m_object_buffer;

    std::string m_name;
};

template <typename T>
class gpu_block_sparse_array
{
public:
    using cpu_type = T;
    using gpu_type = T::gpu_type;

    gpu_block_sparse_array(std::uint32_t level)
        : m_allocator(level)
    {
        m_object_buffer = std::make_unique<structured_buffer>(
            64 * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);
    }

    T& operator[](render_id id)
    {
        return m_objects[id].cpu_data;
    }

    const T& operator[](render_id id) const
    {
        return m_objects[id].cpu_data;
    }

    render_id add(std::uint32_t count = 1)
    {
        render_id id = m_allocator.allocate(count);
        if (id == buddy_allocator::no_space)
        {
            throw std::runtime_error("gpu_block_sparse_array no space.");
        }

        if (m_objects.size() <= id + count - 1)
        {
            m_objects.resize(id + count);
        }

        for (std::uint32_t i = 0; i < count; ++i)
        {
            m_objects[id + i].valid = true;
            mark_dirty(id + i);
        }

        m_objects[id].block_size = count;

        m_size += count;
        m_request_size = std::max(m_request_size, id + count);

        return id;
    }

    void remove(render_id id)
    {
        assert(m_objects[id].block_size != 0);

        std::uint32_t count = m_allocator.get_size(id);
        count = std::min(count, static_cast<std::uint32_t>(m_objects.size() - id));

        for (std::uint32_t i = 0; i < count; ++i)
        {
            m_objects[id + i].block_size = 0;
            m_objects[id + i].valid = false;
            m_objects[id + i].dirty = false;
        }

        m_allocator.free(id);

        m_size -= m_objects[id].block_size;
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

    template <typename Functor>
    void each(Functor&& functor)
    {
        for (render_id id = 0; id < m_objects.size(); ++id)
        {
            if (m_objects[id].valid)
            {
                functor(id, m_objects[id].cpu_data);
            }
        }
    }

    template <typename GetDataFunctor, typename UploadFunctor>
    bool update(GetDataFunctor&& get_data, UploadFunctor&& upload, bool update_all = false)
    {
        if (m_dirty_objects.empty())
        {
            return false;
        }

        bool need_resize = false;

        if (m_request_size > get_capacity())
        {
            reserve(m_request_size);
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
                if (!m_objects[id].valid || !m_objects[id].dirty)
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

    std::uint32_t get_size() const noexcept
    {
        return m_size;
    }

    std::uint32_t get_capacity() const noexcept
    {
        return static_cast<std::uint32_t>(m_object_buffer->get_size() / sizeof(gpu_type));
    }

    structured_buffer* get_buffer() const noexcept
    {
        return m_object_buffer.get();
    }

    void set_name(std::string_view name)
    {
        m_name = name;
        m_object_buffer->get_rhi()->set_name(m_name.c_str());
    }

private:
    struct wrapper
    {
        cpu_type cpu_data;

        std::uint32_t block_size{0};

        bool dirty;
        bool valid;
    };

    void reserve(std::uint32_t object_count)
    {
        std::uint32_t capacity = get_capacity();
        while (capacity < object_count)
        {
            capacity += capacity / 2;
        }

        m_object_buffer = std::make_unique<structured_buffer>(
            capacity * sizeof(gpu_type),
            RHI_BUFFER_STORAGE | RHI_BUFFER_TRANSFER_DST);

        if (!m_name.empty())
        {
            m_object_buffer->get_rhi()->set_name(m_name.c_str());
        }
    }

    std::vector<wrapper> m_objects;
    std::vector<render_id> m_dirty_objects;

    std::uint32_t m_size{0};
    std::uint32_t m_request_size{0};

    buddy_allocator m_allocator;
    std::unique_ptr<structured_buffer> m_object_buffer;

    std::string m_name;
};
} // namespace violet
#pragma once

#include "common/allocator.hpp"
#include "resource/stream_client.hpp"
#include "resource/stream_source.hpp"
#include <memory>

namespace violet
{
class stream_manager
{
public:
    void register_client(stream_client* client);
    void unregister_client(stream_client* client);

    template <typename T, typename... Args>
    std::uint32_t register_source(Args&&... args)
    {
        std::uint32_t id = m_source_allocator.allocate();

        if (id >= m_sources.size())
        {
            m_sources.resize(id + 1);
        }

        m_sources[id] = std::make_unique<T>(std::forward<Args>(args)...);

        return id;
    }

    void unregister_source(std::uint32_t source_id);

    void update();

private:
    std::vector<stream_client*> m_clients;

    std::vector<std::unique_ptr<stream_source>> m_sources;
    index_allocator m_source_allocator;
};
} // namespace violet
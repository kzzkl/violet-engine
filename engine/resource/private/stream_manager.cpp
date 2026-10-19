#include "resource/stream_manager.hpp"
#include <algorithm>

namespace violet
{
void stream_manager::register_client(stream_client* client)
{
    m_clients.push_back(client);
}

void stream_manager::unregister_client(stream_client* client)
{
    auto iter = std::ranges::find(m_clients, client);
    if (iter != m_clients.end())
    {
        m_clients.erase(iter);
    }
}

void stream_manager::unregister_source(std::uint32_t source_id)
{
    m_sources[source_id]->close();
    m_sources[source_id] = nullptr;
    m_source_allocator.free(source_id);
}
} // namespace violet
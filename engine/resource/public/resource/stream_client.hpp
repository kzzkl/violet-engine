#pragma once

#include "resource/stream_types.hpp"
#include <span>
#include <vector>

namespace violet
{
class stream_client
{
public:
    virtual ~stream_client() = default;

    virtual void collect_requests(std::vector<stream_request>& requests) = 0;

    virtual void on_page_loaded(stream_key key, std::span<std::byte> data) = 0;
    virtual void on_page_failed(stream_key key) = 0;
    virtual void on_page_evicted(stream_key key) = 0;
};
} // namespace violet
#pragma once

#include <cstdint>

namespace violet
{
struct stream_key
{
    std::uint32_t source;
    std::uint32_t page;
};

struct stream_request
{
    stream_key key;
    std::uint64_t priority;
};
} // namespace violet
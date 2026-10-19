#pragma once

#include <span>
#include <string_view>

namespace violet
{
class stream_source
{
public:
    virtual ~stream_source() = default;

    virtual bool open(std::string_view uri) = 0;
    virtual void close() = 0;

    virtual bool read(std::size_t offset, std::span<std::byte> out) = 0;

    virtual std::size_t get_size() const = 0;
};
} // namespace violet
#pragma once

#include "resource/stream_source.hpp"
#include <fstream>

namespace violet
{
class file_stream_source : public stream_source
{
public:
    bool open(std::string_view uri) override;

    bool read(std::size_t offset, std::span<std::byte> out) override;

    std::size_t get_size() const override
    {
        return m_size;
    }

private:
    std::string m_path;
    std::size_t m_size{0};

    std::ifstream m_file;
};
} // namespace violet
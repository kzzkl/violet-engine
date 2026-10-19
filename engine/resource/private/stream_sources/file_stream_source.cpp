#include "resource/stream_sources/file_stream_source.hpp"

namespace violet
{
bool file_stream_source::open(std::string_view uri)
{
    if (m_path == uri && m_file.is_open())
    {
        return true;
    }

    m_path = uri;

    m_file.open(std::string(uri));

    if (!m_file.is_open())
    {
        return false;
    }

    m_file.seekg(0, std::ios::end);
    m_size = m_file.tellg();
    m_file.seekg(0, std::ios::beg);

    return m_file.is_open();
}

bool file_stream_source::read(std::size_t offset, std::span<std::byte> out)
{
    if (!m_file.is_open())
    {
        return false;
    }

    m_file.seekg(static_cast<std::streamoff>(offset));
    m_file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));

    return true;
}
} // namespace violet
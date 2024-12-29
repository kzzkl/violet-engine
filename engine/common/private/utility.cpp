#include "common/utility.hpp"

#ifdef _WIN32
#include <Windows.h>
#endif

namespace violet
{
std::wstring string_to_wstring(std::string_view str)
{
#ifdef _WIN32
    int length =
        MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

    std::wstring buffer;
    buffer.resize(length);
    MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        buffer.data(),
        static_cast<int>(buffer.size() * sizeof(wchar_t)));

    return buffer;
#else
    throw std::runtime_error("Not implemented.");
#endif
}

std::string wstring_to_string(std::wstring_view str)
{
#ifdef _WIN32
    char buffer[128] = {};
    WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(wcslen(str.data())),
        buffer,
        static_cast<int>(sizeof(buffer)),
        nullptr,
        nullptr);

    return buffer;
#else
    throw std::runtime_error("Not implemented.");
#endif
}
} // namespace violet
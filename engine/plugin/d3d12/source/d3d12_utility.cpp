#include "d3d12_utility.hpp"
#include <windows.h>

namespace violet::graphics::d3d12
{
std::wstring string_to_wstring(std::string_view str)
{
    wchar_t buffer[128] = {};
    MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        buffer,
        static_cast<int>(sizeof(buffer)));

    return buffer;
}

std::string wstring_to_string(std::wstring_view str)
{
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
}
} // namespace violet::graphics::d3d12
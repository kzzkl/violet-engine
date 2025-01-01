#pragma once

#include <string>

namespace violet
{
std::wstring string_to_wstring(std::string_view str);

std::string wstring_to_string(std::wstring_view str);
} // namespace violet
#pragma once

#include <string>

namespace ash::sample::mmd
{
enum class encode_type
{
    UTF8,
    UTF16,
    SHIFT_JIS
};

template <encode_type encode>
struct encode_string;

template <encode_type encode>
using encode_string_t = encode_string<encode>::type;

template <>
struct encode_string<encode_type::UTF8>
{
    using type = std::string;
};

template <>
struct encode_string<encode_type::UTF16>
{
    using type = std::u16string;
};

template <>
struct encode_string<encode_type::SHIFT_JIS>
{
    using type = std::string;
};

template <encode_type from, encode_type to>
bool convert(const encode_string_t<from>& input, encode_string_t<to>& output);

template <>
bool convert<encode_type::UTF8, encode_type::UTF16>(
    const std::string& input,
    std::u16string& output);

template <>
bool convert<encode_type::UTF16, encode_type::UTF8>(
    const std::u16string& input,
    std::string& output);

template <>
bool convert<encode_type::SHIFT_JIS, encode_type::UTF8>(
    const std::string& input,
    std::string& output);
} // namespace ash::sample::mmd
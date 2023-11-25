#pragma once

#include <string>

namespace violet::sample
{
enum encode_type
{
    ENCODE_TYPE_UTF8,
    ENCODE_TYPE_UTF16,
    ENCODE_TYPE_SHIFT_JIS
};

template <encode_type encode>
struct encode_string;

template <encode_type encode>
using encode_string_t = encode_string<encode>::type;

template <>
struct encode_string<ENCODE_TYPE_UTF8>
{
    using type = std::string;
};

template <>
struct encode_string<ENCODE_TYPE_UTF16>
{
    using type = std::u16string;
};

template <>
struct encode_string<ENCODE_TYPE_SHIFT_JIS>
{
    using type = std::string;
};

template <encode_type from, encode_type to>
bool convert(const encode_string_t<from>& input, encode_string_t<to>& output);

template <>
bool convert<ENCODE_TYPE_UTF8, ENCODE_TYPE_UTF16>(const std::string& input, std::u16string& output);

template <>
bool convert<ENCODE_TYPE_UTF16, ENCODE_TYPE_UTF8>(const std::u16string& input, std::string& output);

template <>
bool convert<ENCODE_TYPE_SHIFT_JIS, ENCODE_TYPE_UTF8>(
    const std::string& input,
    std::string& output);
} // namespace violet::sample
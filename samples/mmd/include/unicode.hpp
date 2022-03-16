#pragma once

#include <string>

namespace ash::sample::mmd
{
enum class encode_type
{
    ENCODE_UTF8,
    ENCODE_UTF16
};

template <encode_type from, encode_type to, typename FromStr, typename ToStr>
bool convert(const FromStr& f, ToStr& t);

template <>
bool convert<encode_type::ENCODE_UTF8, encode_type::ENCODE_UTF16>(
    const std::string& u8Str,
    std::u16string& u16Str);

template <>
bool convert<encode_type::ENCODE_UTF16, encode_type::ENCODE_UTF8>(
    const std::u16string& u16Str,
    std::string& u8Str);
} // namespace ash::sample::mmd
#include "unicode.hpp"
#include <array>

using namespace std;

namespace ash::sample::mmd
{
bool is_u8_later_byte(char ch)
{
    return 0x80 <= std::uint8_t(ch) && std::uint8_t(ch) < 0xC0;
}

bool is_u16_high_surrogate(char16_t ch)
{
    return 0xD800 <= ch && ch < 0xDC00;
}

bool is_u16_low_surrogate(char16_t ch)
{
    return 0xDC00 <= ch && ch < 0xE000;
}

int get_u8_byte_count(char ch)
{
    if (0 <= std::uint8_t(ch) && std::uint8_t(ch) < 0x80)
    {
        return 1;
    }
    if (0xC2 <= std::uint8_t(ch) && std::uint8_t(ch) < 0xE0)
    {
        return 2;
    }
    if (0xE0 <= std::uint8_t(ch) && std::uint8_t(ch) < 0xF0)
    {
        return 3;
    }
    if (0xF0 <= std::uint8_t(ch) && std::uint8_t(ch) < 0xF8)
    {
        return 4;
    }
    return 0;
}

bool ConvChU8ToU32(const std::array<char, 4>& u8Ch, char32_t& u32Ch)
{
    int numBytes = get_u8_byte_count(u8Ch[0]);
    if (numBytes == 0)
    {
        return false;
    }
    switch (numBytes)
    {
    case 1:
        u32Ch = char32_t(std::uint8_t(u8Ch[0]));
        break;
    case 2:
        if (!is_u8_later_byte(u8Ch[1]))
        {
            return false;
        }
        if ((std::uint8_t(u8Ch[0]) & 0x1E) == 0)
        {
            return false;
        }

        u32Ch = char32_t(u8Ch[0] & 0x1F) << 6;
        u32Ch |= char32_t(u8Ch[1] & 0x3F);
        break;
    case 3:
        if (!is_u8_later_byte(u8Ch[1]) || !is_u8_later_byte(u8Ch[2]))
        {
            return false;
        }
        if ((std::uint8_t(u8Ch[0]) & 0x0F) == 0 && (std::uint8_t(u8Ch[1]) & 0x20) == 0)
        {
            return false;
        }

        u32Ch = char32_t(u8Ch[0] & 0x0F) << 12;
        u32Ch |= char32_t(u8Ch[1] & 0x3F) << 6;
        u32Ch |= char32_t(u8Ch[2] & 0x3F);
        break;
    case 4:
        if (!is_u8_later_byte(u8Ch[1]) || !is_u8_later_byte(u8Ch[2]) || !is_u8_later_byte(u8Ch[3]))
        {
            return false;
        }
        if ((std::uint8_t(u8Ch[0]) & 0x07) == 0 && (std::uint8_t(u8Ch[1]) & 0x30) == 0)
        {
            return false;
        }

        u32Ch = char32_t(u8Ch[0] & 0x07) << 18;
        u32Ch |= char32_t(u8Ch[1] & 0x3F) << 12;
        u32Ch |= char32_t(u8Ch[2] & 0x3F) << 6;
        u32Ch |= char32_t(u8Ch[3] & 0x3F);
        break;
    default:
        return false;
    }

    return true;
}

bool ConvChU16ToU32(const std::array<char16_t, 2>& u16Ch, char32_t& u32Ch)
{
    if (is_u16_high_surrogate(u16Ch[0]))
    {
        if (is_u16_low_surrogate(u16Ch[1]))
        {
            u32Ch = 0x10000 + (char32_t(u16Ch[0]) - 0xD800) * 0x400 + (char32_t(u16Ch[1]) - 0xDC00);
        }
        else if (u16Ch[1] == 0)
        {
            u32Ch = u16Ch[0];
        }
        else
        {
            return false;
        }
    }
    else if (is_u16_low_surrogate(u16Ch[0]))
    {
        if (u16Ch[1] == 0)
        {
            u32Ch = u16Ch[0];
        }
        else
        {
            return false;
        }
    }
    else
    {
        u32Ch = u16Ch[0];
    }

    return true;
}

bool ConvChU32ToU8(const char32_t u32Ch, std::array<char, 4>& u8Ch)
{
    if (u32Ch > 0x10FFFF)
    {
        return false;
    }

    if (u32Ch < 128)
    {
        u8Ch[0] = char(u32Ch);
        u8Ch[1] = 0;
        u8Ch[2] = 0;
        u8Ch[3] = 0;
    }
    else if (u32Ch < 2048)
    {
        u8Ch[0] = 0xC0 | char(u32Ch >> 6);
        u8Ch[1] = 0x80 | (char(u32Ch) & 0x3F);
        u8Ch[2] = 0;
        u8Ch[3] = 0;
    }
    else if (u32Ch < 65536)
    {
        u8Ch[0] = 0xE0 | char(u32Ch >> 12);
        u8Ch[1] = 0x80 | (char(u32Ch >> 6) & 0x3F);
        u8Ch[2] = 0x80 | (char(u32Ch) & 0x3F);
        u8Ch[3] = 0;
    }
    else
    {
        u8Ch[0] = 0xF0 | char(u32Ch >> 18);
        u8Ch[1] = 0x80 | (char(u32Ch >> 12) & 0x3F);
        u8Ch[2] = 0x80 | (char(u32Ch >> 6) & 0x3F);
        u8Ch[3] = 0x80 | (char(u32Ch) & 0x3F);
    }

    return true;
}

bool ConvChU32ToU16(const char32_t u32Ch, std::array<char16_t, 2>& u16Ch)
{
    if (u32Ch > 0x10FFFF)
    {
        return false;
    }

    if (u32Ch < 0x10000)
    {
        u16Ch[0] = char16_t(u32Ch);
        u16Ch[1] = 0;
    }
    else
    {
        u16Ch[0] = char16_t((u32Ch - 0x10000) / 0x400 + 0xD800);
        u16Ch[1] = char16_t((u32Ch - 0x10000) % 0x400 + 0xDC00);
    }

    return true;
}

bool ConvChU8ToU16(const std::array<char, 4>& u8Ch, std::array<char16_t, 2>& u16Ch)
{
    char32_t u32Ch;
    if (!ConvChU8ToU32(u8Ch, u32Ch))
    {
        return false;
    }
    if (!ConvChU32ToU16(u32Ch, u16Ch))
    {
        return false;
    }
    return true;
}

bool ConvChU16ToU8(const std::array<char16_t, 2>& u16Ch, std::array<char, 4>& u8Ch)
{
    char32_t u32Ch;
    if (!ConvChU16ToU32(u16Ch, u32Ch))
    {
        return false;
    }
    if (!ConvChU32ToU8(u32Ch, u8Ch))
    {
        return false;
    }
    return true;
}

template <>
bool convert<encode_type::ENCODE_UTF8, encode_type::ENCODE_UTF16>(
    const std::string& u8Str,
    std::u16string& u16Str)
{
    for (auto u8It = u8Str.begin(); u8It != u8Str.end(); ++u8It)
    {
        auto numBytes = get_u8_byte_count((*u8It));
        if (numBytes == 0)
        {
            return false;
        }

        std::array<char, 4> u8Ch;
        u8Ch[0] = (*u8It);
        for (int i = 1; i < numBytes; i++)
        {
            ++u8It;
            if (u8It == u8Str.end())
            {
                return false;
            }
            u8Ch[i] = (*u8It);
        }

        std::array<char16_t, 2> u16Ch;
        if (!ConvChU8ToU16(u8Ch, u16Ch))
        {
            return false;
        }

        u16Str.push_back(u16Ch[0]);
        if (u16Ch[1] != 0)
        {
            u16Str.push_back(u16Ch[1]);
        }
    }
    return true;
}

template <>
bool convert<encode_type::ENCODE_UTF16, encode_type::ENCODE_UTF8>(
    const std::u16string& u16Str,
    std::string& u8Str)
{
    for (auto u16It = u16Str.begin(); u16It != u16Str.end(); ++u16It)
    {
        std::array<char16_t, 2> u16Ch;
        if (is_u16_high_surrogate((*u16It)))
        {
            u16Ch[0] = (*u16It);
            ++u16It;
            if (u16It == u16Str.end())
            {
                return false;
            }
            u16Ch[1] = (*u16It);
        }
        else
        {
            u16Ch[0] = (*u16It);
            u16Ch[1] = 0;
        }

        std::array<char, 4> u8Ch;
        if (!ConvChU16ToU8(u16Ch, u8Ch))
        {
            return false;
        }
        if (u8Ch[0] != 0)
        {
            u8Str.push_back(u8Ch[0]);
        }
        if (u8Ch[1] != 0)
        {
            u8Str.push_back(u8Ch[1]);
        }
        if (u8Ch[2] != 0)
        {
            u8Str.push_back(u8Ch[2]);
        }
        if (u8Ch[3] != 0)
        {
            u8Str.push_back(u8Ch[3]);
        }
    }
    return true;
}
} // namespace sora::unicode
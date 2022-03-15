#pragma once

#include <cstddef>
#include <cstdint>

namespace ash
{
class uuid
{
public:
    using iterator = uint8_t*;
    using const_iterator = const uint8_t*;

public:
    constexpr uuid(const char* str)
    {
        constexpr auto to_uint8_t = [](char c) {
            if (c >= '0' && c <= '9')
                return c - '0';
            else if (c >= 'a' && c <= 'f')
                return c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')
                return c - 'A' + 10;
            else
                return 0;
        };

        std::size_t i = 0;
        const char* p = str;
        while (*p != '\0' && i < 16)
        {
            if (p[0] == '-')
            {
                ++p;
            }
            else
            {
                m_data[i] = 16 * to_uint8_t(p[0]) + to_uint8_t(p[1]);
                ++i;
                p += 2;
            }
        }
    }

    constexpr inline std::size_t hash() const noexcept
    {
        std::size_t hash = 0;
        for (auto iter = cbegin(); iter != cend(); ++iter)
            hash ^= static_cast<std::size_t>(*iter) + 0x9e3779b9 + (hash << 6) + (hash >> 2);

        return hash;
    }

    constexpr inline iterator begin() noexcept { return m_data; }
    constexpr inline iterator end() noexcept { return m_data + size(); }

    constexpr inline const_iterator cbegin() const noexcept { return m_data; }
    constexpr inline const_iterator cend() const noexcept { return m_data + size(); }

    constexpr static inline std::size_t size() noexcept { return 16; }

    constexpr bool operator==(const uuid& other) const noexcept
    {
        for (std::size_t i = 0; i < size(); ++i)
        {
            if (m_data[i] != other.m_data[i])
                return false;
        }
        return true;
    }

private:
    uint8_t m_data[16];
};

struct uuid_hash
{
    constexpr std::size_t operator()(const uuid& key) const noexcept { return key.hash(); }
};
} // namespace ash
#pragma once

#include "core_exports.hpp"
#include "dictionary.hpp"
#include "task_manager.hpp"
#include <string_view>
#include <type_traits>

namespace ash::core
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

    iterator begin() { return m_data; }
    iterator end() { return m_data + size(); }

    const_iterator cbegin() const { return m_data; }
    const_iterator cend() const { return m_data + size(); }

    static constexpr std::size_t size() { return 16; }

    constexpr bool operator==(const uuid& other) const
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
    std::size_t operator()(const uuid& key) const
    {
        std::size_t hash = 0;
        for (auto iter = key.cbegin(); iter != key.cend(); ++iter)
        {
            hash ^= static_cast<std::size_t>(*iter) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
        }

        return hash;
    }
};

class context;
class CORE_API submodule
{
public:
    submodule(std::string_view name);
    virtual ~submodule() = default;

    virtual bool initialize(const ash::common::dictionary& config) = 0;

    std::string_view get_name() const { return m_name; }

protected:
    context& get_context();

private:
    friend class context;

    std::string m_name;
    context* m_context;
};

template <typename T>
concept derived_from_submodule = std::is_base_of<submodule, T>::value;

template <derived_from_submodule T>
struct submodule_trait
{
    static constexpr uuid id = T::id;
};
} // namespace ash::core
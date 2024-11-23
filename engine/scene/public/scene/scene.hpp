#pragma once

#include <cstdint>
#include <string>

namespace violet
{
class scene
{
public:
    scene(std::string_view name, std::uint32_t id)
        : m_name(name),
          m_id(id)
    {
    }

    const std::string& get_name() const noexcept
    {
        return m_name;
    }

    std::uint32_t get_id() const noexcept
    {
        return m_id;
    }

private:
    std::string m_name;
    std::uint32_t m_id{0};
};
} // namespace violet
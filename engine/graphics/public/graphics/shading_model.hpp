#pragma once

#include <string>
#include <vector>

namespace violet
{
class shading_model
{
public:
    shading_model(
        std::string_view name,
        std::size_t constant_size = 0,
        std::string_view shader_path = "");
    virtual ~shading_model() = default;

    const std::vector<std::wstring>& get_defines() const noexcept
    {
        return m_defines;
    }

    const std::string& get_name() const noexcept
    {
        return m_name;
    }

    const void* get_constant_data() const noexcept
    {
        return m_constant.data();
    }

    std::size_t get_constant_size() const noexcept
    {
        return m_constant.size();
    }

protected:
    template <typename T>
    T& get_constant() noexcept
    {
        return *reinterpret_cast<T*>(m_constant.data());
    }

private:
    std::string m_name;

    std::vector<std::uint8_t> m_constant;
    std::vector<std::wstring> m_defines;
};
} // namespace violet
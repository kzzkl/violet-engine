#pragma once

#include <string>

namespace violet
{
class rdg_object
{
public:
    rdg_object() = default;
    rdg_object(const rdg_object&) = delete;
    virtual ~rdg_object() = default;

    rdg_object& operator=(const rdg_object&) = delete;

    virtual void reset() noexcept {}
};

class rdg_node : public rdg_object
{
public:
    rdg_node() = default;
    rdg_node(const rdg_node&) = delete;
    virtual ~rdg_node() = default;

    rdg_node& operator=(const rdg_node&) = delete;

    void set_name(std::string_view name)
    {
        m_name = name;
    }

    const std::string& get_name() const noexcept
    {
        return m_name;
    }

    void cull() noexcept
    {
        m_culled = true;
    }

    bool is_culled() const noexcept
    {
        return m_culled;
    }

    void reset() noexcept override
    {
        m_culled = false;
    }

private:
    std::string m_name;
    bool m_culled{false};
};
} // namespace violet
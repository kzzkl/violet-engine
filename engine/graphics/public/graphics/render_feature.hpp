#pragma once

#include "common/type_index.hpp"
#include <cstdint>

namespace violet
{
struct render_feature_index : public type_index<render_feature_index, std::uint32_t>
{
};

class render_feature_base
{
public:
    virtual ~render_feature_base() = default;

    void update(std::uint32_t width, std::uint32_t height)
    {
        on_update(width, height);
    }

    void set_enable(bool enable) noexcept
    {
        if (m_enable == enable)
        {
            return;
        }

        m_enable = enable;

        if (m_enable)
        {
            on_enable();
        }
        else
        {
            on_disable();
        }
    }

    bool is_enable() const noexcept
    {
        return m_enable;
    }

    virtual std::uint32_t get_id() const noexcept = 0;

private:
    virtual void on_update(std::uint32_t width, std::uint32_t height) {}
    virtual void on_enable() {}
    virtual void on_disable() {}

    bool m_enable{true};
};

template <typename T>
class render_feature : public render_feature_base
{
public:
    std::uint32_t get_id() const noexcept override
    {
        return render_feature_index::value<T>();
    }
};
} // namespace violet
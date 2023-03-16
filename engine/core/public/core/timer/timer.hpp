#pragma once

#include <array>
#include <chrono>
#include <thread>

namespace violet::core
{
class timer
{
public:
    enum point
    {
        PRE_FRAME_START,
        PRE_FRAME_END,
        FRAME_START,
        FRAME_END,
        NUM_TIME_POINT
    };

    using steady_time_point = std::chrono::time_point<std::chrono::steady_clock>;

public:
    timer() {}

    template <typename Clock = std::chrono::steady_clock>
    static std::chrono::time_point<Clock> now()
    {
        return Clock::now();
    }

    template <point P>
    void tick()
    {
        if constexpr (P == FRAME_START)
        {
            m_time_point[PRE_FRAME_START] = m_time_point[FRAME_START];
            m_time_point[PRE_FRAME_END] = m_time_point[FRAME_END];
        }

        m_time_point[P] = now<std::chrono::steady_clock>();
    }

    template <point P>
    inline steady_time_point time_point() const noexcept
    {
        return m_time_point[P];
    }

    template <point S, point E, typename D = std::chrono::nanoseconds>
    inline D delta() const noexcept
    {
        return std::chrono::duration_cast<D>(m_time_point[E] - m_time_point[S]);
    }

    inline float frame_delta() const noexcept
    {
        return delta<PRE_FRAME_START, FRAME_START>().count() * 0.000000001f;
    }

private:
    std::array<steady_time_point, NUM_TIME_POINT> m_time_point;
};
} // namespace violet::core
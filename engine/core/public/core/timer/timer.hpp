#pragma once

#include <array>
#include <chrono>
#include <thread>

namespace violet
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

    void tick(point point)
    {
        if (point == FRAME_START)
        {
            m_time_point[PRE_FRAME_START] = m_time_point[FRAME_START];
            m_time_point[PRE_FRAME_END] = m_time_point[FRAME_END];
        }

        m_time_point[point] = now<std::chrono::steady_clock>();
    }

    inline steady_time_point time_point(point point) const noexcept { return m_time_point[point]; }

    template <typename D = std::chrono::nanoseconds>
    inline D get_delta(point start, point end) const noexcept
    {
        return std::chrono::duration_cast<D>(m_time_point[end] - m_time_point[start]);
    }

    inline float get_frame_delta() const noexcept
    {
        return get_delta(PRE_FRAME_START, FRAME_START).count() * 0.000000001f;
    }

private:
    std::array<steady_time_point, NUM_TIME_POINT> m_time_point;
};
} // namespace violet
#pragma once

#include <chrono>

namespace ash::core
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
    template <typename Clock>
    static std::chrono::time_point<Clock> now()
    {
        return Clock::now();
    }

    template <class Rep, class Period>
    static void busy_sleep(const std::chrono::duration<Rep, Period>& duration)
    {
        auto current = std::chrono::steady_clock::now();
        auto end_time = current + duration;
        while (current < end_time)
        {
            current = std::chrono::steady_clock::now();
            std::this_thread::yield();
        }
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
    std::size_t m_index;
    std::array<steady_time_point, NUM_TIME_POINT> m_time_point;
};
} // namespace ash::core
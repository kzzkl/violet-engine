#pragma once

#include <chrono>

namespace ash::core
{
class timer
{
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
};
} // namespace ash::core
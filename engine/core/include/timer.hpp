#pragma once

#include <chrono>

namespace ash::core
{
using steady_clock = std::chrono::steady_clock;

class timer
{
public:
    template <typename Clock>
    static std::chrono::time_point<Clock> now()
    {
        return Clock::now();
    }
};
} // namespace ash::core
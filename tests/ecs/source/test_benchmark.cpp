#include "test_common.hpp"
#include <chrono>
#include <iostream>

namespace violet::test
{
class timer
{
public:
    void start() noexcept { m_start = std::chrono::steady_clock::now(); }
    double elapse() const noexcept
    {
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - m_start).count();
    }

private:
    std::chrono::steady_clock::time_point m_start;
};

TEST_CASE("Create entities", "[benchmark]")
{
    timer timer;
    world world;
}

TEST_CASE("Iterating entities", "[benchmark]")
{
    timer timer;
    world world;
}

TEST_CASE("Access components", "[benchmark]")
{
    timer timer;
    world world;
}
} // namespace violet::test
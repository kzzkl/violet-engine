#pragma once

#include <catch2/catch.hpp>

namespace test
{
struct position
{
    int x;
    int y;
    int z;
};

struct velocity
{
    int x;
    int y;
    int z;
};

struct rotation
{
    int angle;
};

struct life_counter
{
    int construct;
    int copy_construct;
    int move_construct;

    int assignment;
    int move_assignment;

    int destruct;
};

class test_class
{
public:
    test_class() : m_counter(nullptr) {}
    test_class(life_counter* counter) : m_counter(counter)
    {
        if (m_counter)
            ++m_counter->construct;
    }

    test_class(const test_class& other) : m_counter(other.m_counter)
    {
        if (m_counter)
            ++m_counter->copy_construct;
    }

    test_class(test_class&& other) : m_counter(other.m_counter)
    {
        if (m_counter)
            ++m_counter->move_construct;
    }

    ~test_class()
    {
        if (m_counter)
            ++m_counter->destruct;
    }

    test_class& operator=(const test_class& other)
    {
        if (m_counter)
            ++m_counter->assignment;
        return *this;
    }

    test_class& operator=(test_class&& other)
    {
        if (m_counter)
            ++m_counter->move_assignment;
        return *this;
    }

private:
    life_counter* m_counter;
};
} // namespace Test
#pragma once

#include "ecs/world.hpp"
#include <catch2/catch_test_macros.hpp>

namespace violet::test
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

class geometry;
class material;

struct mesh
{
    geometry* geometry;
    std::uint32_t vertex_offset;
    std::uint32_t vertex_count;
    std::uint32_t index_offset;
    std::uint32_t index_count;
    std::vector<material*> materials;
};

template <std::size_t index>
class life_counter
{
public:
    life_counter()
    {
        ++m_construct;
    }
    life_counter(const life_counter& other)
    {
        ++m_copy_construct;
    }
    life_counter(life_counter&& other) noexcept
    {
        ++m_move_construct;
    }
    ~life_counter()
    {
        ++m_destruct;
    }

    life_counter& operator=(const life_counter& other)
    {
        ++m_assignment;
        return *this;
    }

    life_counter& operator=(life_counter&& other)
    {
        ++m_move_assignment;
        return *this;
    }

    static void reset()
    {
        m_construct = 0;
        m_copy_construct = 0;
        m_move_construct = 0;
        m_assignment = 0;
        m_move_assignment = 0;
        m_destruct = 0;
    }

    static bool check(
        int construct,
        int copy_construct,
        int move_construct,
        int assignment,
        int move_assignment,
        int destruct)
    {
        return m_construct == construct && m_copy_construct == copy_construct &&
               m_move_construct == move_construct && m_assignment == assignment &&
               m_move_assignment == move_assignment && m_destruct == destruct;
    }

private:
    static inline int m_construct{0};
    static inline int m_copy_construct{0};
    static inline int m_move_construct{0};
    static inline int m_assignment{0};
    static inline int m_move_assignment{0};
    static inline int m_destruct{0};
};
} // namespace violet::test
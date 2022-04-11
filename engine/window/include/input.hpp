#pragma once

#include "assert.hpp"
#include <array>
#include <memory>
#include <vector>

namespace ash::window
{
class key_state
{
public:
    explicit key_state(unsigned char state) noexcept : m_state(state) {}

    inline bool down() const noexcept { return m_state & 0x1; }
    inline bool up() const noexcept { return !down(); }
    inline bool release() const noexcept { return m_state == 0x2; }
    inline bool hold() const noexcept { return m_state == 0x3; }

private:
    unsigned char m_state;
};

template <typename KeyType>
class key_device
{
    static const std::uint32_t NUM_KEY = static_cast<std::uint32_t>(KeyType::NUM_TYPE);

public:
    key_device() noexcept { memset(m_key_state, 0, sizeof(m_key_state)); }
    virtual ~key_device() {}

    inline key_state key(KeyType key) const noexcept
    {
        std::size_t index = static_cast<std::uint32_t>(key);
        ASH_ASSERT(index < NUM_KEY);

        return key_state(m_key_state[index]);
    }

    void key_down(KeyType key) noexcept
    {
        std::size_t index = static_cast<std::uint32_t>(key);
        ASH_ASSERT(index < NUM_KEY);

        m_key_state[index] = ((m_key_state[index] << 1) & 0x2) | 0x1;
        m_update_key.push_back(key);
    }

    void key_up(KeyType key) noexcept
    {
        std::size_t index = static_cast<std::uint32_t>(key);
        ASH_ASSERT(index < NUM_KEY);

        m_key_state[index] = (m_key_state[index] << 1) & 0x2;
        m_update_key.push_back(key);
    }

    void tick()
    {
        if (!m_update_key.empty())
        {
            for (KeyType key : m_update_key)
            {
                std::size_t index = static_cast<std::uint32_t>(key);

                std::uint8_t old = m_key_state[index] & 0x1;
                m_key_state[index] = (m_key_state[index] << 1) | old;
            }
            m_update_key.clear();
        }
    }

private:
    std::vector<KeyType> m_update_key;
    std::uint8_t m_key_state[NUM_KEY];
};

enum class mouse_mode : std::uint8_t
{
    CURSOR_ABSOLUTE = 0,
    CURSOR_RELATIVE
};

enum class mouse_key : std::uint32_t
{
    LEFT_BUTTON,
    RIGHT_BUTTON,
    MIDDLE_BUTTON,
    NUM_TYPE
};

class mouse : public key_device<mouse_key>
{
public:
    mouse() noexcept;
    virtual ~mouse() = default;

    void mode(mouse_mode mode);
    inline mouse_mode mode() const noexcept { return m_mode; }

    inline int x() const noexcept { return m_x; }
    inline int y() const noexcept { return m_y; }

protected:
    virtual void change_mode(mouse_mode mode) = 0;

    int m_x;
    int m_y;

    mouse_mode m_mode;
};

enum class keyboard_key : std::uint32_t
{
    KEY_ESC = 0x1B,
    KEY_0 = 0x30,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A = 0x41,
    KEY_B,
    KEY_C,
    KEY_D,
    KEY_E,
    KEY_F,
    KEY_G,
    KEY_H,
    KEY_I,
    KEY_J,
    KEY_K,
    KEY_L,
    KEY_M,
    KEY_N,
    KEY_O,
    KEY_P,
    KEY_Q,
    KEY_R,
    KEY_S,
    KEY_T,
    KEY_U,
    KEY_V,
    KEY_W,
    KEY_X,
    KEY_Y,
    KEY_Z,
    NUM_TYPE
};

class keyboard : public key_device<keyboard_key>
{
public:
    keyboard() noexcept;
};
} // namespace ash::window
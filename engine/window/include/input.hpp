#pragma once

#include "window_exports.hpp"
#include <array>
#include <memory>

namespace ash::window
{
class WINDOW_API key_state
{
public:
    explicit key_state(unsigned char state) : m_state(state) {}

    inline bool down() const { return m_state & 0x1; }
    inline bool up() const { return !down(); }
    inline bool release() const { return m_state == 0x2; }
    inline bool hold() const { return m_state == 0x3; }

private:
    unsigned char m_state;
};

template <typename KeyType>
class key_device
{
    static const std::uint32_t NUM_KEY = static_cast<std::uint32_t>(KeyType::NUM_TYPE);

public:
    key_device() { memset(m_key_state, 0, sizeof(m_key_state)); }
    virtual ~key_device() {}

    inline key_state key(KeyType key) { return key_state(m_key_state[static_cast<std::uint32_t>(key)]); }

    void key_down(KeyType key)
    {
        std::uint32_t k = static_cast<std::uint32_t>(key);
        if (static_cast<std::uint32_t>(k) < NUM_KEY)
        {
            auto oldState = m_key_state[k];
            m_key_state[k] = ((m_key_state[k] << 1) & 0x2) | 0x1;
        }
    }

    void key_up(KeyType key)
    {
        std::uint32_t k = static_cast<std::uint32_t>(key);
        if (k < NUM_KEY)
        {
            auto oldState = m_key_state[k];
            m_key_state[k] = (m_key_state[k] << 1) & 0x2;
        }
    }

private:
    unsigned char m_key_state[NUM_KEY];
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

class WINDOW_API mouse : public key_device<mouse_key>
{
public:
    mouse();
    virtual ~mouse() = default;

    void reset_relative_cursor();

    void set_cursor(int x, int y)
    {
        m_x = x;
        m_y = y;
    }

    void set_mode(mouse_mode mode);
    inline mouse_mode get_mode() const { return m_mode; }

    inline int get_x() const { return m_x; }
    inline int get_y() const { return m_y; }

protected:
    virtual void clip_cursor(bool clip) = 0;
    virtual void show_cursor(bool show) = 0;

private:
    void set_cursor_clip(bool clip);

    int m_x;
    int m_y;
    mouse_mode m_mode;
};

enum class keyboard_key : std::uint32_t
{
    KEY_0 = 48,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A = 65,
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

class WINDOW_API keyboard : public key_device<keyboard_key>
{
public:
    keyboard();
};
} // namespace ash::window
#pragma once

#include <array>
#include <cassert>
#include <memory>
#include <vector>

namespace violet
{
class key_state
{
public:
    key_state() : m_state(0) {}
    explicit key_state(unsigned char state) noexcept : m_state(state) {}

    inline bool down() const noexcept { return m_state & 0x1; }
    inline bool up() const noexcept { return !down(); }

    inline bool press() const noexcept { return m_state == 0x1; }
    inline bool release() const noexcept { return m_state == 0x2; }
    inline bool hold() const noexcept { return m_state == 0x3; }

private:
    unsigned char m_state;
};

template <typename KeyType, std::size_t KeyCount>
class key_device
{
public:
    key_device() noexcept { memset(m_key_state, 0, sizeof(m_key_state)); }
    virtual ~key_device() {}

    inline key_state key(KeyType key) const noexcept
    {
        std::size_t index = static_cast<std::uint32_t>(key);
        assert(index < KeyCount);

        return key_state(m_key_state[index]);
    }

    void key_down(KeyType key) noexcept
    {
        std::size_t index = static_cast<std::uint32_t>(key);
        assert(index < KeyCount);

        m_key_state[index] = ((m_key_state[index] << 1) & 0x2) | 0x1;
        m_update_key.push_back(key);
    }

    void key_up(KeyType key) noexcept
    {
        std::size_t index = static_cast<std::uint32_t>(key);
        assert(index < KeyCount);

        m_key_state[index] = (m_key_state[index] << 1) & 0x2;
        m_update_key.push_back(key);
    }

    virtual void tick()
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
    std::uint8_t m_key_state[KeyCount];
};

enum mouse_mode
{
    MOUSE_MODE_ABSOLUTE,
    MOUSE_MODE_RELATIVE
};

enum mouse_cursor
{
    MOUSE_CURSOR_ARROW,
    MOUSE_CURSOR_SIZE_NWSE,
    MOUSE_CURSOR_SIZE_NESW,
    MOUSE_CURSOR_SIZE_WE,
    MOUSE_CURSOR_SIZE_NS,
    MOUSE_CURSOR_SIZE_ALL
};

enum mouse_key : uint8_t
{
    MOUSE_KEY_LEFT,
    MOUSE_KEY_RIGHT,
    MOUSE_KEY_MIDDLE,
    MOUSE_KEY_COUNT
};

class window_impl;
class mouse : public key_device<mouse_key, MOUSE_KEY_COUNT>
{
public:
    using device_type = key_device<mouse_key, MOUSE_KEY_COUNT>;

public:
    mouse(window_impl* impl) noexcept;
    virtual ~mouse() = default;

    void set_mode(mouse_mode mode);
    mouse_mode get_mode() const noexcept;
    void set_cursor(mouse_cursor cursor);

    inline int get_x() const noexcept { return m_x; }
    inline int get_y() const noexcept { return m_y; }
    inline int get_wheel() const noexcept { return m_wheel; }

    virtual void tick() override;

protected:
    friend class window_system;
    int m_x;
    int m_y;
    int m_wheel;

    window_impl* m_impl;
};

enum keyboard_key
{
    KEYBOARD_KEY_BACK,
    KEYBOARD_KEY_TAB,
    KEYBOARD_KEY_RETURN,
    KEYBOARD_KEY_PAUSE,
    KEYBOARD_KEY_CAPITAL,
    KEYBOARD_KEY_ESCAPE,
    KEYBOARD_KEY_SPACE,
    KEYBOARD_KEY_PRIOR,
    KEYBOARD_KEY_NEXT,
    KEYBOARD_KEY_END,
    KEYBOARD_KEY_HOME,
    KEYBOARD_KEY_LEFT,
    KEYBOARD_KEY_UP,
    KEYBOARD_KEY_RIGHT,
    KEYBOARD_KEY_DOWN,
    KEYBOARD_KEY_SNAPSHOT,
    KEYBOARD_KEY_INSERT,
    KEYBOARD_KEY_DELETE,
    KEYBOARD_KEY_0,
    KEYBOARD_KEY_1,
    KEYBOARD_KEY_2,
    KEYBOARD_KEY_3,
    KEYBOARD_KEY_4,
    KEYBOARD_KEY_5,
    KEYBOARD_KEY_6,
    KEYBOARD_KEY_7,
    KEYBOARD_KEY_8,
    KEYBOARD_KEY_9,
    KEYBOARD_KEY_A,
    KEYBOARD_KEY_B,
    KEYBOARD_KEY_C,
    KEYBOARD_KEY_D,
    KEYBOARD_KEY_E,
    KEYBOARD_KEY_F,
    KEYBOARD_KEY_G,
    KEYBOARD_KEY_H,
    KEYBOARD_KEY_I,
    KEYBOARD_KEY_J,
    KEYBOARD_KEY_K,
    KEYBOARD_KEY_L,
    KEYBOARD_KEY_M,
    KEYBOARD_KEY_N,
    KEYBOARD_KEY_O,
    KEYBOARD_KEY_P,
    KEYBOARD_KEY_Q,
    KEYBOARD_KEY_R,
    KEYBOARD_KEY_S,
    KEYBOARD_KEY_T,
    KEYBOARD_KEY_U,
    KEYBOARD_KEY_V,
    KEYBOARD_KEY_W,
    KEYBOARD_KEY_X,
    KEYBOARD_KEY_Y,
    KEYBOARD_KEY_Z,
    KEYBOARD_KEY_LWIN,
    KEYBOARD_KEY_RWIN,
    KEYBOARD_KEY_APPS,
    KEYBOARD_KEY_NUMPAD0,
    KEYBOARD_KEY_NUMPAD1,
    KEYBOARD_KEY_NUMPAD2,
    KEYBOARD_KEY_NUMPAD3,
    KEYBOARD_KEY_NUMPAD4,
    KEYBOARD_KEY_NUMPAD5,
    KEYBOARD_KEY_NUMPAD6,
    KEYBOARD_KEY_NUMPAD7,
    KEYBOARD_KEY_NUMPAD8,
    KEYBOARD_KEY_NUMPAD9,
    KEYBOARD_KEY_MULTIPLY,
    KEYBOARD_KEY_ADD,
    KEYBOARD_KEY_SUBTRACT,
    KEYBOARD_KEY_DECIMAL,
    KEYBOARD_KEY_DIVIDE,
    KEYBOARD_KEY_F1,
    KEYBOARD_KEY_F2,
    KEYBOARD_KEY_F3,
    KEYBOARD_KEY_F4,
    KEYBOARD_KEY_F5,
    KEYBOARD_KEY_F6,
    KEYBOARD_KEY_F7,
    KEYBOARD_KEY_F8,
    KEYBOARD_KEY_F9,
    KEYBOARD_KEY_F10,
    KEYBOARD_KEY_F11,
    KEYBOARD_KEY_F12,
    KEYBOARD_KEY_NUMLOCK,
    KEYBOARD_KEY_SCROLL,
    KEYBOARD_KEY_LSHIFT,
    KEYBOARD_KEY_RSHIFT,
    KEYBOARD_KEY_LCONTROL,
    KEYBOARD_KEY_RCONTROL,
    KEYBOARD_KEY_LMENU,
    KEYBOARD_KEY_RMENU,
    KEYBOARD_KEY_OEM_1,
    KEYBOARD_KEY_OEM_PLUS,
    KEYBOARD_KEY_OEM_COMMA,
    KEYBOARD_KEY_OEM_MINUS,
    KEYBOARD_KEY_OEM_PERIOD,
    KEYBOARD_KEY_OEM_2,
    KEYBOARD_KEY_OEM_3,
    KEYBOARD_KEY_OEM_4,
    KEYBOARD_KEY_OEM_5,
    KEYBOARD_KEY_OEM_6,
    KEYBOARD_KEY_OEM_7,
    KEYBOARD_KEY_COUNT
};

class keyboard : public key_device<keyboard_key, KEYBOARD_KEY_COUNT>
{
public:
    keyboard() noexcept;
};
} // namespace violet
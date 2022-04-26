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

    inline bool press() const noexcept { return m_state == 0x1; }
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

class window_impl;
class mouse : public key_device<mouse_key>
{
public:
    mouse(window_impl* impl) noexcept;
    virtual ~mouse() = default;

    void mode(mouse_mode mode);
    inline mouse_mode mode() const noexcept { return m_mode; }

    inline int x() const noexcept { return m_x; }
    inline int y() const noexcept { return m_y; }
    inline int whell() const noexcept { return m_whell; }

    virtual void tick() override;

protected:
    friend class window;
    int m_x;
    int m_y;
    int m_whell;

    mouse_mode m_mode;
    window_impl* m_impl;
};

enum class keyboard_key : std::uint32_t
{
    KEY_BACK,
    KEY_TAB,
    KEY_RETURN,
    KEY_PAUSE,
    KEY_CAPITAL,
    KEY_ESCAPE,
    KEY_SPACE,
    KEY_PRIOR,
    KEY_NEXT,
    KEY_END,
    KEY_HOME,
    KEY_LEFT,
    KEY_UP,
    KEY_RIGHT,
    KEY_DOWN,
    KEY_SNAPSHOT,
    KEY_INSERT,
    KEY_DELETE,
    KEY_0,
    KEY_1,
    KEY_2,
    KEY_3,
    KEY_4,
    KEY_5,
    KEY_6,
    KEY_7,
    KEY_8,
    KEY_9,
    KEY_A,
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
    KEY_LWIN,
    KEY_RWIN,
    KEY_APPS,
    KEY_NUMPAD0,
    KEY_NUMPAD1,
    KEY_NUMPAD2,
    KEY_NUMPAD3,
    KEY_NUMPAD4,
    KEY_NUMPAD5,
    KEY_NUMPAD6,
    KEY_NUMPAD7,
    KEY_NUMPAD8,
    KEY_NUMPAD9,
    KEY_MULTIPLY,
    KEY_ADD,
    KEY_SUBTRACT,
    KEY_DECIMAL,
    KEY_DIVIDE,
    KEY_F1,
    KEY_F2,
    KEY_F3,
    KEY_F4,
    KEY_F5,
    KEY_F6,
    KEY_F7,
    KEY_F8,
    KEY_F9,
    KEY_F10,
    KEY_F11,
    KEY_F12,
    KEY_NUMLOCK,
    KEY_SCROLL,
    KEY_LSHIFT,
    KEY_RSHIFT,
    KEY_LCONTROL,
    KEY_RCONTROL,
    KEY_LMENU,
    KEY_RMENU,
    KEY_OEM_1,
    KEY_OEM_PLUS,
    KEY_OEM_COMMA,
    KEY_OEM_MINUS,
    KEY_OEM_PERIOD,
    KEY_OEM_2,
    KEY_OEM_3,
    KEY_OEM_4,
    KEY_OEM_5,
    KEY_OEM_6,
    KEY_OEM_7,
    NUM_TYPE
};

class keyboard : public key_device<keyboard_key>
{
public:
    keyboard() noexcept;
};
} // namespace ash::window
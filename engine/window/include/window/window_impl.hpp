#pragma once

#include "window/input.hpp"
#include <string_view>

namespace ash::window
{
struct window_extent
{
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t width;
    std::uint32_t height;
};

struct window_message
{
    enum class message_type
    {
        MOUSE_MOVE,
        MOUSE_KEY,
        MOUSE_WHELL,
        KEYBOARD_KEY,
        KEYBOARD_CHAR,
        WINDOW_MOVE,
        WINDOW_RESIZE
    } type;

    union {
        struct
        {
            int x;
            int y;
        } mouse_move;

        struct
        {
            mouse_key key;
            bool down;
        } mouse_key;

        int mouse_whell;

        struct
        {
            keyboard_key key;
            bool down;
        } keyboard_key;

        char keyboard_char;

        struct
        {
            int x;
            int y;
        } window_move;

        struct
        {
            std::uint32_t width;
            std::uint32_t height;
        } window_resize;
    };
};

class window_impl
{
public:
    using mouse_type = mouse;
    using mouse_mode_type = mouse_mode;
    using mouse_cursor_type = mouse_cursor;
    using keyboard_type = keyboard;

public:
    virtual ~window_impl() = default;

    virtual bool initialize(std::uint32_t width, std::uint32_t height, std::string_view title) = 0;
    virtual void shutdown() {}

    virtual void tick() = 0;
    virtual void show() = 0;

    virtual void* handle() const = 0;
    virtual window_extent extent() const = 0;

    virtual void title(std::string_view title) = 0;

    virtual void mouse_mode(mouse_mode_type mode) = 0;
    virtual void mouse_cursor(mouse_cursor_type cursor) = 0;

    virtual mouse_mode_type mouse_mode() const noexcept = 0;

    void reset() noexcept { m_messages.clear(); }
    const std::vector<window_message>& messages() const noexcept { return m_messages; }

protected:
    std::vector<window_message> m_messages;
};
} // namespace ash::window
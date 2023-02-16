#pragma once

#include "window_impl.hpp"
#include <Windows.h>
#include <windowsx.h>

namespace violet::window
{
class window_impl_win32 : public window_impl
{
public:
    window_impl_win32() noexcept;

    virtual bool initialize(std::uint32_t width, std::uint32_t height, std::string_view title)
        override;
    virtual void shutdown() override;

    virtual void tick() override;
    virtual void show() override;

    virtual void* handle() const override;
    virtual window_extent extent() const override;

    virtual void title(std::string_view title) override;

    virtual void mouse_mode(mouse_mode_type mode) override;
    virtual void mouse_cursor(mouse_cursor_type cursor) override;

    virtual mouse_mode_type mouse_mode() const noexcept override { return m_mouse_mode; }

private:
    static LRESULT CALLBACK wnd_create_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    void on_mouse_move(int x, int y);
    void on_mouse_key(mouse_key key, bool down);
    void on_mouse_whell(int value);
    void on_keyboard_key(keyboard_key key, bool down);
    void on_keyboard_char(char c);
    void on_window_move(int x, int y);
    void on_window_resize(std::uint32_t width, std::uint32_t height);

    static constexpr auto* m_class_name = L"violet-engine";

    mouse_mode_type m_mouse_mode;
    bool m_mouse_mode_change;

    mouse_cursor_type m_mouse_cursor;
    std::vector<HCURSOR> m_mouse_cursor_handles;

    int m_mouse_x;
    int m_mouse_y;
    bool m_mouse_move;

    int m_mouse_whell;
    bool m_mouse_whell_move;

    std::uint32_t m_window_width;
    std::uint32_t m_window_height;
    bool m_window_resize;

    HINSTANCE m_instance;
    HWND m_hwnd;

    bool m_track_mouse_event_flag;
};
} // namespace violet::window
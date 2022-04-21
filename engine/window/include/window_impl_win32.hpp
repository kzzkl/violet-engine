#pragma once

#include "window_impl.hpp"
#include <Windows.h>
#include <windowsx.h>

namespace ash::window
{
class window_impl_win32 : public window_impl
{
public:
    window_impl_win32() noexcept;

    virtual bool initialize(std::uint32_t width, std::uint32_t height, std::string_view title)
        override;
    virtual void tick() override;
    virtual void show() override;

    virtual void* handle() const override;
    virtual window_rect rect() const override;

    virtual void title(std::string_view title) override;
    virtual void change_mouse_mode(mouse_mode mode) override;

private:
    static LRESULT CALLBACK wnd_create_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    void on_mouse_move(int x, int y);
    void on_mouse_key(mouse_key key, bool down);
    void on_keyboard_key(keyboard_key key, bool down);
    void on_window_move(int x, int y);
    void on_window_resize(int width, int height);

    static constexpr auto* m_class_name = L"ash-engine";

    bool m_mouse_mode_change;
    mouse_mode m_mouse_mode;
    int m_mouse_x;
    int m_mouse_y;
    bool m_mouse_move;

    HINSTANCE m_instance;
    HWND m_hwnd;
};
} // namespace ash::window
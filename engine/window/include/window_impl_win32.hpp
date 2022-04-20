#pragma once

#include "window_impl.hpp"
#include <Windows.h>
#include <windowsx.h>

namespace ash::window
{
class mouse_win32 : public mouse
{
public:
    void reset();
    void cursor(int x, int y)
    {
        m_x = x;
        m_y = y;
    }

    void window_handle(HWND hwnd) { m_hwnd = hwnd; }

private:
    virtual void change_mode(mouse_mode mode) override;

    HWND m_hwnd;
    bool m_mode_change;
};

class window_impl_win32 : public window_impl
{
public:
    virtual bool initialize(std::uint32_t width, std::uint32_t height, std::string_view title)
        override;
    virtual void tick() override;
    virtual void show() override;

    virtual void* handle() const override;
    virtual window_rect rect() const override;

    virtual mouse_type& mouse() override { return m_mouse; };
    virtual keyboard_type& keyboard() override { return m_keyboard; };

    virtual void title(std::string_view title) override;

private:
    static LRESULT CALLBACK wnd_create_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    static constexpr auto* m_class_name = L"ash-engine";

    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    HINSTANCE m_instance;
    HWND m_hwnd;

    mouse_win32 m_mouse;
    keyboard_type m_keyboard;
};
} // namespace ash::window
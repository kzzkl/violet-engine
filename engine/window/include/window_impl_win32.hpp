#pragma once

#include "window_impl.hpp"
#include <Windows.h>
#include <windowsx.h>

namespace ash::window
{
class mouse_win32 : public mouse
{
public:
    void set_window_handle(HWND hwnd) { m_hwnd = hwnd; }

protected:
    virtual void clip_cursor(bool clip) override;
    virtual void show_cursor(bool show) override;

private:
    HWND m_hwnd;
};

class window_impl_win32 : public window_impl
{
public:
    virtual bool initialize(uint32_t width, uint32_t height, std::string_view title) override;
    virtual void tick() override;
    virtual void show() override;

    virtual const void* get_handle() const override;
    virtual window_rect get_rect() const override;

    virtual mouse& get_mouse() override { return m_mouse; };

    virtual void set_title(std::string_view title) override;

private:
    static LRESULT CALLBACK wnd_create_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    constexpr static const auto* m_class_name = L"ash-engine";

    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    HINSTANCE m_instance;
    HWND m_hwnd;

    mouse_win32 m_mouse;
    keyboard m_keyboard;
};
} // namespace ash::window
#pragma once

#include "window_impl.hpp"
#include <Windows.h>
#include <windowsx.h>

namespace violet
{
class window_win32 : public window_impl
{
public:
    window_win32() noexcept;
    virtual ~window_win32();

    bool initialize(std::uint32_t width, std::uint32_t height, std::string_view title) override;
    void shutdown() override;

    void tick() override;
    void show() override;

    void* get_handle() const override;

    rect<std::uint32_t> get_window_size() const override;
    rect<std::uint32_t> get_screen_size() const override;

    void set_title(std::string_view title) override;

    void set_mouse_mode(mouse_mode_type mode) override;
    mouse_mode_type get_mouse_mode() const noexcept override
    {
        return m_mouse_mode;
    }

    void set_cursor(mouse_cursor_type cursor) override;

    void set_cursor_position(const vec2i& position) override;

    vec2i get_screen_position(const vec2i& window_position) const override;

private:
    static LRESULT CALLBACK wnd_create_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    static LRESULT CALLBACK wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);
    LRESULT handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam);

    void on_mouse_move(int x, int y);
    void on_mouse_key(mouse_key key, bool down);
    void on_mouse_wheel(int value);
    void on_keyboard_key(keyboard_key key, bool down);
    void on_keyboard_char(char c);
    void on_window_move(int x, int y);
    void on_window_resize(std::uint32_t width, std::uint32_t height);
    void on_window_destroy();

    static constexpr auto* m_class_name = L"violet-engine";

    mouse_mode_type m_mouse_mode{MOUSE_MODE_ABSOLUTE};
    bool m_mouse_mode_change{false};

    mouse_cursor_type m_mouse_cursor{MOUSE_CURSOR_ARROW};
    std::vector<HCURSOR> m_mouse_cursor_handles;

    vec2i m_mouse_position{};
    bool m_mouse_move{false};

    int m_mouse_wheel{0};
    bool m_mouse_wheel_move{false};

    vec2u m_window_size{};
    bool m_window_resize{false};
    bool m_window_destroy{false};

    HINSTANCE m_instance;
    HWND m_hwnd{nullptr};
};
} // namespace violet
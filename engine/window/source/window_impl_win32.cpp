#include "window_impl_win32.hpp"
#include "assert.hpp"
#include "log.hpp"

namespace ash::window
{
std::wstring string_to_wstring(std::string_view str)
{
    wchar_t buffer[128] = {};
    MultiByteToWideChar(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(str.size()),
        buffer,
        static_cast<int>(sizeof(buffer)));

    return buffer;
}

std::string wstring_to_string(std::wstring_view str)
{
    char buffer[128] = {};
    WideCharToMultiByte(
        CP_UTF8,
        0,
        str.data(),
        static_cast<int>(wcslen(str.data())),
        buffer,
        static_cast<int>(sizeof(buffer)),
        nullptr,
        nullptr);

    return buffer;
}

window_impl_win32::window_impl_win32() noexcept
    : m_mouse_mode_change(false),
      m_mouse_mode(mouse_mode::CURSOR_ABSOLUTE),
      m_mouse_x(0),
      m_mouse_y(0),
      m_mouse_move(false)
{
}

bool window_impl_win32::initialize(
    std::uint32_t width,
    std::uint32_t height,
    std::string_view title)
{
    m_instance = GetModuleHandle(0);

    WNDCLASSEX wndClassEx;
    memset(&wndClassEx, 0, sizeof(wndClassEx));
    wndClassEx.cbSize = sizeof(WNDCLASSEX);
    wndClassEx.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wndClassEx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClassEx.hIcon = LoadIcon(m_instance, IDI_APPLICATION);
    wndClassEx.hInstance = m_instance;
    wndClassEx.lpfnWndProc = wnd_create_proc;
    wndClassEx.lpszClassName = m_class_name;
    wndClassEx.lpszMenuName = NULL;
    wndClassEx.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    RegisterClassEx(&wndClassEx);

    RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

    SetProcessDPIAware();

    ASH_ASSERT(title.size() < 128, "The title is too long");
    std::wstring wtitle = string_to_wstring(title);

    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        wndClassEx.lpszClassName,
        wtitle.c_str(),
        WS_OVERLAPPEDWINDOW,
        0,
        0,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_instance,
        this);

    // Center window.
    int system_dpi = GetDpiForSystem();
    int window_dpi = GetDpiForWindow(m_hwnd);
    int pos_x = (GetSystemMetrics(SM_CXSCREEN) -
                 (windowRect.right - windowRect.left) * window_dpi / system_dpi) /
                2;
    int pos_y = (GetSystemMetrics(SM_CYSCREEN) -
                 (windowRect.bottom - windowRect.top) * window_dpi / system_dpi) /
                2;
    SetWindowPos(m_hwnd, 0, pos_x, pos_y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x1;
    rid.usUsage = 0x2;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = m_hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        log::error("RegisterRawInputDevices failed");
    }

    show();

    return true;
}

void window_impl_win32::tick()
{
    if (m_mouse_mode_change)
    {
        if (m_mouse_mode == mouse_mode::CURSOR_ABSOLUTE)
        {
            ShowCursor(true);
            ClipCursor(nullptr);
        }
        else
        {
            ShowCursor(false);
            RECT rect;
            GetClientRect(m_hwnd, &rect);
            MapWindowRect(m_hwnd, nullptr, &rect);
            ClipCursor(&rect);
        }
        m_mouse_mode_change = false;
    }

    m_mouse_x = m_mouse_y = 0;
    m_mouse_move = false;
    m_mouse_whell = 0;
    m_mouse_whell_move = false;

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (m_mouse_move)
    {
        window_message message;
        message.type = window_message::message_type::MOUSE_MOVE;
        message.mouse_move.x = m_mouse_x;
        message.mouse_move.y = m_mouse_y;
        m_messages.push_back(message);
    }

    if (m_mouse_whell_move)
    {
        window_message message;
        message.type = window_message::message_type::MOUSE_WHELL;
        message.mouse_whell.value = m_mouse_whell;
        m_messages.push_back(message);
    }
}

void window_impl_win32::show()
{
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
}

void* window_impl_win32::handle() const
{
    return m_hwnd;
}

window_rect window_impl_win32::rect() const
{
    RECT rect = {};
    GetClientRect(m_hwnd, &rect);

    window_rect result = {};
    result.x = rect.left;
    result.y = rect.top;
    result.width = rect.right - rect.left;
    result.height = rect.bottom - rect.top;

    return result;
}

void window_impl_win32::title(std::string_view title)
{
    SetWindowText(m_hwnd, string_to_wstring(title).c_str());
}

void window_impl_win32::change_mouse_mode(mouse_mode mode)
{
    m_mouse_mode = mode;
    m_mouse_mode_change = true;
}

LRESULT CALLBACK
window_impl_win32::wnd_create_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* create_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
        window_impl_win32* impl =
            reinterpret_cast<window_impl_win32*>(create_struct->lpCreateParams);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
        SetWindowLongPtr(
            hwnd,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&window_impl_win32::wnd_proc));
        return impl->handle_message(hwnd, message, wparam, lparam);
    }
    else
    {
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
}

LRESULT CALLBACK window_impl_win32::wnd_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    window_impl_win32* const impl =
        reinterpret_cast<window_impl_win32*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    return impl->handle_message(hwnd, message, wparam, lparam);
}

LRESULT window_impl_win32::handle_message(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    switch (message)
    {
    case WM_MOUSEMOVE: {
        if (m_mouse_mode == mouse_mode::CURSOR_ABSOLUTE)
            on_mouse_move(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        break;
    }
    case WM_INPUT: {
        if (m_mouse_mode == mouse_mode::CURSOR_RELATIVE)
        {
            RAWINPUT raw;
            UINT raw_size = sizeof(raw);
            GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lparam),
                RID_INPUT,
                &raw,
                &raw_size,
                sizeof(RAWINPUTHEADER));
            if (raw.header.dwType == RIM_TYPEMOUSE)
                on_mouse_move(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        on_mouse_key(mouse_key::LEFT_BUTTON, true);
        break;
    }
    case WM_LBUTTONUP: {
        on_mouse_key(mouse_key::LEFT_BUTTON, false);
        break;
    }
    case WM_RBUTTONDOWN: {
        on_mouse_key(mouse_key::RIGHT_BUTTON, true);
        break;
    }
    case WM_RBUTTONUP: {
        on_mouse_key(mouse_key::RIGHT_BUTTON, false);
        break;
    }
    case WM_MBUTTONDOWN: {
        on_mouse_key(mouse_key::MIDDLE_BUTTON, true);
        break;
    }
    case WM_MBUTTONUP: {
        on_mouse_key(mouse_key::MIDDLE_BUTTON, false);
        break;
    }
    case WM_MOUSEWHEEL: {
        on_mouse_whell(static_cast<short>(HIWORD(wparam)) / 120);
        break;
    }
    case WM_KEYDOWN: {
        on_keyboard_key(static_cast<keyboard_key>(wparam), true);
        break;
    }
    case WM_KEYUP: {
        on_keyboard_key(static_cast<keyboard_key>(wparam), false);
        break;
    }
    default:
        return DefWindowProc(hwnd, message, wparam, lparam);
    }
    return 0;
}

void window_impl_win32::on_mouse_move(int x, int y)
{
    if (m_mouse_mode == mouse_mode::CURSOR_ABSOLUTE)
    {
        m_mouse_x = x;
        m_mouse_y = y;
    }
    else
    {
        m_mouse_x += x;
        m_mouse_y += y;
    }
    m_mouse_move = true;
}

void window_impl_win32::on_mouse_key(mouse_key key, bool down)
{
    window_message message;
    message.type = window_message::message_type::MOUSE_KEY;
    message.mouse_key.key = key;
    message.mouse_key.down = down;
    m_messages.push_back(message);
}

void window_impl_win32::on_mouse_whell(int value)
{
    m_mouse_whell += value;
    m_mouse_whell_move = true;
}

void window_impl_win32::on_keyboard_key(keyboard_key key, bool down)
{
    window_message message;
    message.type = window_message::message_type::KEYBOARD_KEY;
    message.keyboard_key.key = key;
    message.keyboard_key.down = down;
    m_messages.push_back(message);
}

void window_impl_win32::on_window_move(int x, int y)
{
    window_message message;
    message.type = window_message::message_type::WINDOW_MOVE;
    message.window_move.x = x;
    message.window_move.y = y;
    m_messages.push_back(message);
}

void window_impl_win32::on_window_resize(int width, int height)
{
    window_message message;
    message.type = window_message::message_type::WINDOW_RESIZE;
    message.window_resize.width = width;
    message.window_resize.height = height;
    m_messages.push_back(message);
}
} // namespace ash::window
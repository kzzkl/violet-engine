#include "window/window_impl_win32.hpp"
#include "assert.hpp"
#include "log.hpp"

namespace ash::window
{
namespace
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

keyboard_key convert_key(WPARAM wparam)
{
    switch (wparam)
    {
    case VK_BACK:
        return keyboard_key::KEY_BACK;
    case VK_TAB:
        return keyboard_key::KEY_TAB;
    case VK_RETURN:
        return keyboard_key::KEY_RETURN;
    case VK_PAUSE:
        return keyboard_key::KEY_PAUSE;
    case VK_CAPITAL:
        return keyboard_key::KEY_CAPITAL;
    case VK_ESCAPE:
        return keyboard_key::KEY_ESCAPE;
    case VK_SPACE:
        return keyboard_key::KEY_SPACE;
    case VK_PRIOR:
        return keyboard_key::KEY_PRIOR;
    case VK_NEXT:
        return keyboard_key::KEY_NEXT;
    case VK_END:
        return keyboard_key::KEY_END;
    case VK_HOME:
        return keyboard_key::KEY_HOME;
    case VK_LEFT:
        return keyboard_key::KEY_LEFT;
    case VK_UP:
        return keyboard_key::KEY_UP;
    case VK_RIGHT:
        return keyboard_key::KEY_RIGHT;
    case VK_DOWN:
        return keyboard_key::KEY_DOWN;
    case VK_SNAPSHOT:
        return keyboard_key::KEY_SNAPSHOT;
    case VK_INSERT:
        return keyboard_key::KEY_INSERT;
    case VK_DELETE:
        return keyboard_key::KEY_DELETE;
    case '0':
        return keyboard_key::KEY_0;
    case '1':
        return keyboard_key::KEY_1;
    case '2':
        return keyboard_key::KEY_2;
    case '3':
        return keyboard_key::KEY_3;
    case '4':
        return keyboard_key::KEY_4;
    case '5':
        return keyboard_key::KEY_5;
    case '6':
        return keyboard_key::KEY_6;
    case '7':
        return keyboard_key::KEY_7;
    case '8':
        return keyboard_key::KEY_8;
    case '9':
        return keyboard_key::KEY_9;
    case 'A':
        return keyboard_key::KEY_A;
    case 'B':
        return keyboard_key::KEY_B;
    case 'C':
        return keyboard_key::KEY_C;
    case 'D':
        return keyboard_key::KEY_D;
    case 'E':
        return keyboard_key::KEY_E;
    case 'F':
        return keyboard_key::KEY_F;
    case 'G':
        return keyboard_key::KEY_G;
    case 'H':
        return keyboard_key::KEY_H;
    case 'I':
        return keyboard_key::KEY_I;
    case 'J':
        return keyboard_key::KEY_J;
    case 'K':
        return keyboard_key::KEY_K;
    case 'L':
        return keyboard_key::KEY_L;
    case 'M':
        return keyboard_key::KEY_M;
    case 'N':
        return keyboard_key::KEY_N;
    case 'O':
        return keyboard_key::KEY_O;
    case 'P':
        return keyboard_key::KEY_P;
    case 'Q':
        return keyboard_key::KEY_Q;
    case 'R':
        return keyboard_key::KEY_R;
    case 'S':
        return keyboard_key::KEY_S;
    case 'T':
        return keyboard_key::KEY_T;
    case 'U':
        return keyboard_key::KEY_U;
    case 'V':
        return keyboard_key::KEY_V;
    case 'W':
        return keyboard_key::KEY_W;
    case 'X':
        return keyboard_key::KEY_X;
    case 'Y':
        return keyboard_key::KEY_Y;
    case 'Z':
        return keyboard_key::KEY_Z;
    case VK_LWIN:
        return keyboard_key::KEY_LWIN;
    case VK_RWIN:
        return keyboard_key::KEY_RWIN;
    case VK_APPS:
        return keyboard_key::KEY_APPS;
    case VK_NUMPAD0:
        return keyboard_key::KEY_NUMPAD0;
    case VK_NUMPAD1:
        return keyboard_key::KEY_NUMPAD1;
    case VK_NUMPAD2:
        return keyboard_key::KEY_NUMPAD2;
    case VK_NUMPAD3:
        return keyboard_key::KEY_NUMPAD3;
    case VK_NUMPAD4:
        return keyboard_key::KEY_NUMPAD4;
    case VK_NUMPAD5:
        return keyboard_key::KEY_NUMPAD5;
    case VK_NUMPAD6:
        return keyboard_key::KEY_NUMPAD6;
    case VK_NUMPAD7:
        return keyboard_key::KEY_NUMPAD7;
    case VK_NUMPAD8:
        return keyboard_key::KEY_NUMPAD8;
    case VK_NUMPAD9:
        return keyboard_key::KEY_NUMPAD9;
    case VK_MULTIPLY:
        return keyboard_key::KEY_MULTIPLY;
    case VK_ADD:
        return keyboard_key::KEY_ADD;
    case VK_SUBTRACT:
        return keyboard_key::KEY_SUBTRACT;
    case VK_DECIMAL:
        return keyboard_key::KEY_DECIMAL;
    case VK_DIVIDE:
        return keyboard_key::KEY_DIVIDE;
    case VK_F1:
        return keyboard_key::KEY_F1;
    case VK_F2:
        return keyboard_key::KEY_F2;
    case VK_F3:
        return keyboard_key::KEY_F3;
    case VK_F4:
        return keyboard_key::KEY_F4;
    case VK_F5:
        return keyboard_key::KEY_F5;
    case VK_F6:
        return keyboard_key::KEY_F6;
    case VK_F7:
        return keyboard_key::KEY_F7;
    case VK_F8:
        return keyboard_key::KEY_F8;
    case VK_F9:
        return keyboard_key::KEY_F9;
    case VK_F10:
        return keyboard_key::KEY_F10;
    case VK_F11:
        return keyboard_key::KEY_F11;
    case VK_F12:
        return keyboard_key::KEY_F12;
    case VK_NUMLOCK:
        return keyboard_key::KEY_NUMLOCK;
    case VK_SCROLL:
        return keyboard_key::KEY_SCROLL;
    case VK_LSHIFT:
        return keyboard_key::KEY_LSHIFT;
    case VK_RSHIFT:
        return keyboard_key::KEY_RSHIFT;
    case VK_LCONTROL:
        return keyboard_key::KEY_LCONTROL;
    case VK_RCONTROL:
        return keyboard_key::KEY_RCONTROL;
    case VK_LMENU:
        return keyboard_key::KEY_LMENU;
    case VK_RMENU:
        return keyboard_key::KEY_RMENU;
    case VK_OEM_1:
        return keyboard_key::KEY_OEM_1;
    case VK_OEM_PLUS:
        return keyboard_key::KEY_OEM_PLUS;
    case VK_OEM_COMMA:
        return keyboard_key::KEY_OEM_COMMA;
    case VK_OEM_MINUS:
        return keyboard_key::KEY_OEM_MINUS;
    case VK_OEM_PERIOD:
        return keyboard_key::KEY_OEM_PERIOD;
    case VK_OEM_2:
        return keyboard_key::KEY_OEM_2;
    case VK_OEM_3:
        return keyboard_key::KEY_OEM_3;
    case VK_OEM_4:
        return keyboard_key::KEY_OEM_4;
    case VK_OEM_5:
        return keyboard_key::KEY_OEM_5;
    case VK_OEM_6:
        return keyboard_key::KEY_OEM_6;
    case VK_OEM_7:
        return keyboard_key::KEY_OEM_7;
    default:
        return keyboard_key::NUM_TYPE;
    }
};

} // namespace

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

    SetProcessDPIAware();

    RECT windowRect = {0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, false);

    ASH_ASSERT(title.size() < 128, "The title is too long");
    std::wstring wtitle = string_to_wstring(title);

    m_hwnd = CreateWindow(
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

    m_window_width = width;
    m_window_height = height;

    show();

    return true;
}

void window_impl_win32::shutdown()
{
    PostMessage(m_hwnd, WM_CLOSE, 0, 0);
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
        message.mouse_whell = m_mouse_whell;
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

window_extent window_impl_win32::extent() const
{
    RECT extent = {};
    GetClientRect(m_hwnd, &extent);

    window_extent result = {};
    result.x = extent.left;
    result.y = extent.top;
    result.width = extent.right - extent.left;
    result.height = extent.bottom - extent.top;

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
    static bool resize_flag = false;

    switch (message)
    {
    case WM_MOUSEMOVE: {
        if (m_mouse_mode == mouse_mode::CURSOR_ABSOLUTE)
            on_mouse_move(GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam));
        break;
    }
    case WM_ENTERSIZEMOVE: {
        break;
    }
    case WM_EXITSIZEMOVE: {
        if (resize_flag)
        {
            on_window_resize(m_window_width, m_window_height);
            resize_flag = false;
        }
        break;
    }
    case WM_SIZE: {
        resize_flag = true;
        m_window_width = LOWORD(lparam);
        m_window_height = HIWORD(lparam);

        if (wparam == SIZE_MAXIMIZED)
        {
            on_window_resize(m_window_width, m_window_height);
            resize_flag = false;
        }
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
        keyboard_key key = convert_key(wparam);
        if (key != keyboard_key::NUM_TYPE)
            on_keyboard_key(key, true);
        break;
    }
    case WM_KEYUP: {
        keyboard_key key = convert_key(wparam);
        if (key != keyboard_key::NUM_TYPE)
            on_keyboard_key(key, false);
        break;
    }
    case WM_CHAR: {
        on_keyboard_char(static_cast<char>(wparam));
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

void window_impl_win32::on_keyboard_char(char c)
{
    window_message message;
    message.type = window_message::message_type::KEYBOARD_CHAR;
    message.keyboard_char = c;
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

void window_impl_win32::on_window_resize(std::uint32_t width, std::uint32_t height)
{
    window_message message;
    message.type = window_message::message_type::WINDOW_RESIZE;
    message.window_resize.width = width;
    message.window_resize.height = height;
    m_messages.push_back(message);
}
} // namespace ash::window
#include "window_impl_win32.hpp"
#include "assert.hpp"
#include "log.hpp"

using namespace ash::common;

namespace ash::window
{
void mouse_win32::clip_cursor(bool clip)
{
    if (clip)
    {
        RECT rect;
        GetClientRect(m_hwnd, &rect);
        MapWindowRect(m_hwnd, nullptr, &rect);
        ClipCursor(&rect);
    }
    else
    {
        ClipCursor(nullptr);
    }
}

void mouse_win32::show_cursor(bool show)
{
    ShowCursor(show);
}

bool window_impl_win32::initialize(uint32_t width, uint32_t height, std::string_view title)
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

    int posX = (GetSystemMetrics(SM_CXSCREEN) - (windowRect.right - windowRect.left)) / 2;
    int posY = (GetSystemMetrics(SM_CYSCREEN) - (windowRect.bottom - windowRect.top)) / 2;

    SetProcessDPIAware();

    ASH_ASSERT(title.size() < 128, "The title is too long");
    WCHAR wTitle[128] = {};
    MultiByteToWideChar(CP_UTF8, 0, title.data(), static_cast<int>(title.size()), wTitle, 128);

    m_hwnd = CreateWindowEx(
        WS_EX_APPWINDOW,
        wndClassEx.lpszClassName,
        wTitle,
        WS_OVERLAPPEDWINDOW,
        posX,
        posY,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        m_instance,
        this);

    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x1;
    rid.usUsage = 0x2;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = m_hwnd;
    if (!RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE)))
    {
        log::error("RegisterRawInputDevices failed");
    }

    m_mouse.set_window_handle(m_hwnd);

    show();

    return true;
}

void window_impl_win32::tick()
{
    m_mouse.reset_relative_cursor();

    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void window_impl_win32::show()
{
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);
}

void* window_impl_win32::get_handle()
{
    return &m_hwnd;
}

LRESULT CALLBACK
window_impl_win32::wnd_create_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE)
    {
        CREATESTRUCT* createStruct = reinterpret_cast<CREATESTRUCT*>(lParam);
        window_impl_win32* impl =
            reinterpret_cast<window_impl_win32*>(createStruct->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(impl));
        SetWindowLongPtr(
            hWnd,
            GWLP_WNDPROC,
            reinterpret_cast<LONG_PTR>(&window_impl_win32::wnd_proc));
        return impl->HandleMsg(hWnd, message, wParam, lParam);
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK window_impl_win32::wnd_proc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    window_impl_win32* const impl =
        reinterpret_cast<window_impl_win32*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    return impl->HandleMsg(hWnd, message, wParam, lParam);
}

LRESULT window_impl_win32::HandleMsg(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_MOUSEMOVE: {
        if (m_mouse.get_mode() == mouse_mode::CURSOR_ABSOLUTE)
            m_mouse.set_cursor(static_cast<int>(LOWORD(lParam)), static_cast<int>(HIWORD(lParam)));
        break;
    }
    case WM_INPUT: {
        if (m_mouse.get_mode() == mouse_mode::CURSOR_RELATIVE)
        {
            RAWINPUT raw;
            UINT rawSize = sizeof(raw);
            GetRawInputData(
                reinterpret_cast<HRAWINPUT>(lParam),
                RID_INPUT,
                &raw,
                &rawSize,
                sizeof(RAWINPUTHEADER));
            if (raw.header.dwType == RIM_TYPEMOUSE &&
                !(raw.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE))
            {
                m_mouse.set_cursor(raw.data.mouse.lLastX, raw.data.mouse.lLastY);
            }
        }
        break;
    }
    case WM_LBUTTONDOWN:
        m_mouse.key_down(mouse_key::LEFT_BUTTON);
        break;
    case WM_LBUTTONUP:
        m_mouse.key_up(mouse_key::LEFT_BUTTON);
        break;
    case WM_RBUTTONDOWN:
        m_mouse.key_down(mouse_key::RIGHT_BUTTON);
        break;
    case WM_RBUTTONUP:
        m_mouse.key_up(mouse_key::RIGHT_BUTTON);
        break;
    case WM_MBUTTONDOWN:
        m_mouse.key_down(mouse_key::MIDDLE_BUTTON);
        break;
    case WM_MBUTTONUP:
        m_mouse.key_up(mouse_key::MIDDLE_BUTTON);
        break;
    case WM_KEYDOWN:
        m_keyboard.key_down(static_cast<keyboard_key>(wParam));
        break;
    case WM_KEYUP:
        m_keyboard.key_up(static_cast<keyboard_key>(wParam));
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
} // namespace ash::window
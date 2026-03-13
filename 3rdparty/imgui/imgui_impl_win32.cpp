// Adapted from thprac_gui_impl_win32.cpp for native th06 integration.
// Original: thprac/thprac/src/thprac/thprac_gui_impl_win32.cpp
// Changes:
//   1. Removed thprac_hook.h dependency entirely
//   2. Removed HookWndProc / UnHookWndProc (not needed in source build)
//   3. Functions renamed to ImGui_ImplWin32_* standard naming
//   4. Removed SetNoClose, CheckFullScreen helpers (unused in th06)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include "imgui_impl_win32.h"
#include <imgui.h>
#include <tchar.h>
#include <windows.h>

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL 0x020E
#endif
#ifndef DBT_DEVNODES_CHANGED
#define DBT_DEVNODES_CHANGED 0x0007
#endif

// ---- State ----
static HWND              g_hWnd = NULL;
static INT64             g_Time = 0;
static INT64             g_TicksPerSecond = 0;
static ImGuiMouseCursor  g_LastMouseCursor = ImGuiMouseCursor_COUNT;
static bool              g_WantUpdateHasGamepad = true;

struct WndKeyStatus {
    unsigned int hold  = 0;
    int          frame = 0;
    char         name[32] = {};
};
static WndKeyStatus  g_wndKeyStatus[256]      = {};
static unsigned int  g_wndKeyFrameStatus[256] = {};

// ---- Key tracking ----
void ImGui_ImplWin32_UpdateKeyState(UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (wParam >= 256) return;
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        g_wndKeyStatus[wParam].hold += (lParam & 0x000000ff);
        g_wndKeyStatus[wParam].frame = 1;
        if (!g_wndKeyStatus[wParam].name[0])
            GetKeyNameTextA(lParam, g_wndKeyStatus[wParam].name, 32);
    } else if (msg == WM_KEYUP || msg == WM_SYSKEYUP) {
        g_wndKeyStatus[wParam].hold = 0;
    }
}

static void ImplWin32_UpdKeyFrame()
{
    for (int i = 0; i < 256; ++i) {
        if (g_wndKeyStatus[i].hold || g_wndKeyStatus[i].frame)
            g_wndKeyFrameStatus[i]++;
        else
            g_wndKeyFrameStatus[i] = 0;
        g_wndKeyStatus[i].frame = 0;
    }
}

int ImGui_ImplWin32_GetKey(int vk)
{
    return g_wndKeyStatus[vk].hold;
}

int ImGui_ImplWin32_GetKeyFrame(int vk)
{
    return g_wndKeyFrameStatus[vk];
}

// ---- Cursor / mouse helpers ----
static bool ImplWin32_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return false;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor) {
        ::SetCursor(NULL);
    } else {
        LPTSTR win32_cursor = IDC_ARROW;
        switch (imgui_cursor) {
        case ImGuiMouseCursor_TextInput:   win32_cursor = IDC_IBEAM;    break;
        case ImGuiMouseCursor_ResizeAll:   win32_cursor = IDC_SIZEALL;  break;
        case ImGuiMouseCursor_ResizeEW:    win32_cursor = IDC_SIZEWE;   break;
        case ImGuiMouseCursor_ResizeNS:    win32_cursor = IDC_SIZENS;   break;
        case ImGuiMouseCursor_ResizeNESW:  win32_cursor = IDC_SIZENESW; break;
        case ImGuiMouseCursor_ResizeNWSE:  win32_cursor = IDC_SIZENWSE; break;
        case ImGuiMouseCursor_Hand:        win32_cursor = IDC_HAND;     break;
        default: break;
        }
        ::SetCursor(::LoadCursor(NULL, win32_cursor));
    }
    return true;
}

static void ImplWin32_UpdateMousePos()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantSetMousePos) {
        POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
        ::ClientToScreen(g_hWnd, &pos);
        ::SetCursorPos(pos.x, pos.y);
    }

    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    POINT pos;
    if (HWND active_window = ::GetForegroundWindow()) {
        if (active_window == g_hWnd || ::IsChild(active_window, g_hWnd)) {
            if (::GetCursorPos(&pos) && ::ScreenToClient(g_hWnd, &pos)) {
                // Map from actual window size to game render size (640x480)
                RECT rect = {};
                GetClientRect(g_hWnd, &rect);
                int wndW = rect.right  - rect.left;
                int wndH = rect.bottom - rect.top;
                if (wndW > 0 && wndH > 0) {
                    io.MousePos.x = (float)pos.x / (float)wndW * io.DisplaySize.x;
                    io.MousePos.y = (float)pos.y / (float)wndH * io.DisplaySize.y;
                }
            }
        }
    }
}

// ---- API ----
bool ImGui_ImplWin32_Init(void* hwnd)
{
    if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&g_TicksPerSecond))
        return false;
    if (!::QueryPerformanceCounter((LARGE_INTEGER*)&g_Time))
        return false;

    g_hWnd = (HWND)hwnd;
    ImGuiIO& io = ImGui::GetIO();
    io.BackendFlags        |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags        |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName  = "imgui_impl_win32";

    io.KeyMap[ImGuiKey_LeftArrow]  = VK_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]    = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow]  = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp]     = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown]   = VK_NEXT;
    io.KeyMap[ImGuiKey_Home]       = VK_HOME;
    io.KeyMap[ImGuiKey_End]        = VK_END;
    io.KeyMap[ImGuiKey_Insert]     = VK_INSERT;
    io.KeyMap[ImGuiKey_Delete]     = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace]  = VK_BACK;
    io.KeyMap[ImGuiKey_Space]      = VK_SPACE;
    io.KeyMap[ImGuiKey_Enter]      = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape]     = VK_ESCAPE;
    io.KeyMap[ImGuiKey_Tab]        = VK_TAB;
    io.KeyMap[ImGuiKey_A]          = 'A';
    io.KeyMap[ImGuiKey_C]          = 'C';
    io.KeyMap[ImGuiKey_V]          = 'V';
    io.KeyMap[ImGuiKey_X]          = 'X';
    io.KeyMap[ImGuiKey_Y]          = 'Y';
    io.KeyMap[ImGuiKey_Z]          = 'Z';
    return true;
}

void ImGui_ImplWin32_Shutdown()
{
    g_hWnd = NULL;
}

void ImGui_ImplWin32_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built!");

    // Display size = game render resolution (always 640x480)
    io.DisplaySize = ImVec2(640.0f, 480.0f);

    // Delta time
    INT64 current_time = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
    io.DeltaTime = (float)(current_time - g_Time) / (float)g_TicksPerSecond;
    g_Time = current_time;

    ImplWin32_UpdateMousePos();

    ImGuiMouseCursor mouse_cursor = io.MouseDrawCursor ? ImGuiMouseCursor_None : ImGui::GetMouseCursor();
    if (g_LastMouseCursor != mouse_cursor) {
        g_LastMouseCursor = mouse_cursor;
        ImplWin32_UpdateMouseCursor();
    }

    ImplWin32_UpdKeyFrame();
}

LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui::GetCurrentContext() == NULL)
        return 0;

    // Track key state for ThpracGui hotkey detection
    ImGui_ImplWin32_UpdateKeyState(msg, wParam, lParam);

    ImGuiIO& io = ImGui::GetIO();
    switch (msg) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK: {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) button = 0;
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) button = 1;
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) button = 2;
        if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK)
            button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(hwnd);
        io.MouseDown[button] = true;
        return 0;
    }
    case WM_LBUTTONUP: case WM_RBUTTONUP: case WM_MBUTTONUP: case WM_XBUTTONUP: {
        int button = 0;
        if (msg == WM_LBUTTONUP) button = 0;
        if (msg == WM_RBUTTONUP) button = 1;
        if (msg == WM_MBUTTONUP) button = 2;
        if (msg == WM_XBUTTONUP) button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
            ::ReleaseCapture();
        return 0;
    }
    case WM_MOUSEWHEEL:
        io.MouseWheel  += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
    case WM_KEYDOWN: case WM_SYSKEYDOWN:
        if (wParam < 256) io.KeysDown[wParam] = 1;
        return 0;
    case WM_KEYUP: case WM_SYSKEYUP:
        if (wParam < 256) io.KeysDown[wParam] = 0;
        return 0;
    case WM_CHAR:
        io.AddInputCharacter((unsigned int)wParam);
        return 0;
    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT && ImplWin32_UpdateMouseCursor())
            return 1;
        return 0;
    case WM_DEVICECHANGE:
        if ((UINT)wParam == DBT_DEVNODES_CHANGED)
            g_WantUpdateHasGamepad = true;
        return 0;
    }
    return 0;
}

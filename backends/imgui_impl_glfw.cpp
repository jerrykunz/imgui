// dear imgui: Platform Backend for GLFW
// This needs to be used along with a Renderer (e.g. OpenGL3, Vulkan, WebGPU..)
// (Info: GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (Requires: GLFW 3.1+. Prefer GLFW 3.3+ for full feature support.)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen/Pen (Windows only).
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent() function. Pass ImGuiKey values to all key functions e.g. ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy GLFW_KEY_* values will also be supported unless IMGUI_DISABLE_OBSOLETE_KEYIO is set]
//  [X] Platform: Gamepad support. Enable with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility. Disable with 'io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange' (note: the resizing cursors requires GLFW 3.4+).
//  [X] Platform: Keyboard arrays indexed using GLFW_KEY_* codes, e.g. ImGui::IsKeyPressed(GLFW_KEY_SPACE).
//  [X] Platform: Multi-viewport support (multiple windows). Enable with 'io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable'.

// Issues:
//  [ ] Platform: Multi-viewport support: ParentViewportID not honored, and so io.ConfigViewportsNoDefaultParent has no effect (minor).

// You can copy and use unmodified imgui_impl_* files in your project. See examples/ folder for examples of using this.
// If you are new to Dear ImGui, read documentation from the docs/ folder + read the top of imgui.cpp.
// Read online: https://github.com/ocornut/imgui/tree/master/docs

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2020-XX-XX: Platform: Added support for multiple windows via the ImGuiPlatformIO interface.
//  2020-01-17: Inputs: Disable error callback while assigning mouse cursors because some X11 setup don't have them and it generates errors.
//  2019-12-05: Inputs: Added support for new mouse cursors added in GLFW 3.4+ (resizing cursors, not allowed cursor).
//  2019-10-18: Misc: Previously installed user callbacks are now restored on shutdown.
//  2019-07-21: Inputs: Added mapping for ImGuiKey_KeyPadEnter.
//  2019-05-11: Inputs: Don't filter value from character callback before calling AddInputCharacter().
//  2019-03-12: Misc: Preserve DisplayFramebufferScale when main window is minimized.
//  2018-11-30: Misc: Setting up io.BackendPlatformName so it can be displayed in the About Window.
//  2018-11-07: Inputs: When installing our GLFW callbacks, we save user's previously installed ones - if any - and chain call them.
//  2018-08-01: Inputs: Workaround for Emscripten which doesn't seem to handle focus related calls.
//  2018-06-29: Inputs: Added support for the ImGuiMouseCursor_Hand cursor.
//  2018-06-08: Misc: Extracted imgui_impl_glfw.cpp/.h away from the old combined GLFW+OpenGL/Vulkan examples.
//  2018-03-20: Misc: Setup io.BackendFlags ImGuiBackendFlags_HasMouseCursors flag + honor ImGuiConfigFlags_NoMouseCursorChange flag.
//  2018-02-20: Inputs: Added support for mouse cursors (ImGui::GetMouseCursor() value, passed to glfwSetCursor()).
//  2018-02-06: Misc: Removed call to ImGui::Shutdown() which is not available from 1.60 WIP, user needs to call CreateContext/DestroyContext themselves.
//  2018-02-06: Inputs: Added mapping for ImGuiKey_Space.
//  2018-01-25: Inputs: Added gamepad support if ImGuiConfigFlags_NavEnableGamepad is set.
//  2018-01-25: Inputs: Honoring the io.WantSetMousePos by repositioning the mouse (when using navigation and ImGuiConfigFlags_NavMoveMouse is set).
//  2018-01-20: Inputs: Added Horizontal Mouse Wheel support.
//  2018-01-18: Inputs: Added mapping for ImGuiKey_Insert.
//  2017-08-25: Inputs: MousePos set to -FLT_MAX,-FLT_MAX when mouse is unavailable/missing (instead of -1,-1).
//  2016-10-15: Misc: Added a void* user_data parameter to Clipboard function handlers.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_glfw.h"

// Clang warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"     // warning: use of old-style cast
#pragma clang diagnostic ignored "-Wsign-conversion"    // warning: implicit conversion changes signedness
#endif

// GLFW
#include <GLFW/glfw3.h>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window()
#endif
#define GLFW_HAS_WINDOW_TOPMOST       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ GLFW_FLOATING
#define GLFW_HAS_WINDOW_HOVERED       (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ GLFW_HOVERED
#define GLFW_HAS_WINDOW_ALPHA         (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwSetWindowOpacity
#define GLFW_HAS_PER_MONITOR_DPI      (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetMonitorContentScale
#define GLFW_HAS_VULKAN               (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwCreateWindowSurface
#define GLFW_HAS_FOCUS_WINDOW         (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3200) // 3.2+ glfwFocusWindow
#define GLFW_HAS_FOCUS_ON_SHOW        (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ GLFW_FOCUS_ON_SHOW
#define GLFW_HAS_MONITOR_WORK_AREA    (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3300) // 3.3+ glfwGetMonitorWorkarea
#define GLFW_HAS_OSX_WINDOW_POS_FIX   (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 + GLFW_VERSION_REVISION * 10 >= 3310) // 3.3.1+ Fixed: Resizing window repositions it on MacOS #1553
#ifdef GLFW_RESIZE_NESW_CURSOR        // Let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2019-11-29 (cursors defines) // FIXME: Remove when GLFW 3.4 is released?
#define GLFW_HAS_NEW_CURSORS          (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3400) // 3.4+ GLFW_RESIZE_ALL_CURSOR, GLFW_RESIZE_NESW_CURSOR, GLFW_RESIZE_NWSE_CURSOR, GLFW_NOT_ALLOWED_CURSOR
#else
#define GLFW_HAS_NEW_CURSORS            (0)
#endif
#ifdef GLFW_MOUSE_PASSTHROUGH         // Let's be nice to people who pulled GLFW between 2019-04-16 (3.4 define) and 2020-07-17 (passthrough)
#define GLFW_HAS_MOUSE_PASSTHROUGH    (GLFW_VERSION_MAJOR * 1000 + GLFW_VERSION_MINOR * 100 >= 3400) // 3.4+ GLFW_MOUSE_PASSTHROUGH
#else
#define GLFW_HAS_MOUSE_PASSTHROUGH    (0)
#endif

// GLFW data
enum GlfwClientApi
{
    GlfwClientApi_Unknown,
    GlfwClientApi_OpenGL,
    GlfwClientApi_Vulkan,
};
static GLFWwindow*          g_Window = NULL;    // Main window
static GlfwClientApi        g_ClientApi = GlfwClientApi_Unknown;
static double               g_Time = 0.0;
static bool                 g_MouseJustPressed[ImGuiMouseButton_COUNT] = {};
static GLFWcursor*          g_MouseCursors[ImGuiMouseCursor_COUNT] = {};
static bool                 g_InstalledCallbacks = false;
static bool                 g_WantUpdateMonitors = true;

// Chain GLFW callbacks for main viewport: our callbacks will call the user's previously installed callbacks, if any.
static GLFWmousebuttonfun   g_PrevUserCallbackMousebutton = NULL;
static GLFWscrollfun        g_PrevUserCallbackScroll = NULL;
static GLFWkeyfun           g_PrevUserCallbackKey = NULL;
static GLFWcharfun          g_PrevUserCallbackChar = NULL;
static GLFWmonitorfun       g_PrevUserCallbackMonitor = NULL;

// Forward Declarations
static void ImGui_ImplGlfw_UpdateMonitors();
static void ImGui_ImplGlfw_InitPlatformInterface();
static void ImGui_ImplGlfw_ShutdownPlatformInterface();

static const char* ImGui_ImplGlfw_GetClipboardText(void* user_data)
{
    return glfwGetClipboardString((GLFWwindow*)user_data);
}

static void ImGui_ImplGlfw_SetClipboardText(void* user_data, const char* text)
{
    glfwSetClipboardString((GLFWwindow*)user_data, text);
}

static ImGuiKey ImGui_ImplGlfw_KeyToImGuiKey(int key)
{
    switch (key)
    {
        case GLFW_KEY_TAB: return ImGuiKey_Tab;
        case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        case GLFW_KEY_PAGE_UP: return ImGuiKey_PageUp;
        case GLFW_KEY_PAGE_DOWN: return ImGuiKey_PageDown;
        case GLFW_KEY_HOME: return ImGuiKey_Home;
        case GLFW_KEY_END: return ImGuiKey_End;
        case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        case GLFW_KEY_SPACE: return ImGuiKey_Space;
        case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        case GLFW_KEY_APOSTROPHE: return ImGuiKey_Apostrophe;
        case GLFW_KEY_COMMA: return ImGuiKey_Comma;
        case GLFW_KEY_MINUS: return ImGuiKey_Minus;
        case GLFW_KEY_PERIOD: return ImGuiKey_Period;
        case GLFW_KEY_SLASH: return ImGuiKey_Slash;
        case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        case GLFW_KEY_LEFT_BRACKET: return ImGuiKey_LeftBracket;
        case GLFW_KEY_BACKSLASH: return ImGuiKey_Backslash;
        case GLFW_KEY_RIGHT_BRACKET: return ImGuiKey_RightBracket;
        case GLFW_KEY_GRAVE_ACCENT: return ImGuiKey_GraveAccent;
        case GLFW_KEY_CAPS_LOCK: return ImGuiKey_CapsLock;
        case GLFW_KEY_SCROLL_LOCK: return ImGuiKey_ScrollLock;
        case GLFW_KEY_NUM_LOCK: return ImGuiKey_NumLock;
        case GLFW_KEY_PRINT_SCREEN: return ImGuiKey_PrintScreen;
        case GLFW_KEY_PAUSE: return ImGuiKey_Pause;
        case GLFW_KEY_KP_0: return ImGuiKey_Keypad0;
        case GLFW_KEY_KP_1: return ImGuiKey_Keypad1;
        case GLFW_KEY_KP_2: return ImGuiKey_Keypad2;
        case GLFW_KEY_KP_3: return ImGuiKey_Keypad3;
        case GLFW_KEY_KP_4: return ImGuiKey_Keypad4;
        case GLFW_KEY_KP_5: return ImGuiKey_Keypad5;
        case GLFW_KEY_KP_6: return ImGuiKey_Keypad6;
        case GLFW_KEY_KP_7: return ImGuiKey_Keypad7;
        case GLFW_KEY_KP_8: return ImGuiKey_Keypad8;
        case GLFW_KEY_KP_9: return ImGuiKey_Keypad9;
        case GLFW_KEY_KP_DECIMAL: return ImGuiKey_KeypadDecimal;
        case GLFW_KEY_KP_DIVIDE: return ImGuiKey_KeypadDivide;
        case GLFW_KEY_KP_MULTIPLY: return ImGuiKey_KeypadMultiply;
        case GLFW_KEY_KP_SUBTRACT: return ImGuiKey_KeypadSubtract;
        case GLFW_KEY_KP_ADD: return ImGuiKey_KeypadAdd;
        case GLFW_KEY_KP_ENTER: return ImGuiKey_KeypadEnter;
        case GLFW_KEY_KP_EQUAL: return ImGuiKey_KeypadEqual;
        case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        case GLFW_KEY_MENU: return ImGuiKey_Menu;
        case GLFW_KEY_0: return ImGuiKey_0;
        case GLFW_KEY_1: return ImGuiKey_1;
        case GLFW_KEY_2: return ImGuiKey_2;
        case GLFW_KEY_3: return ImGuiKey_3;
        case GLFW_KEY_4: return ImGuiKey_4;
        case GLFW_KEY_5: return ImGuiKey_5;
        case GLFW_KEY_6: return ImGuiKey_6;
        case GLFW_KEY_7: return ImGuiKey_7;
        case GLFW_KEY_8: return ImGuiKey_8;
        case GLFW_KEY_9: return ImGuiKey_9;
        case GLFW_KEY_A: return ImGuiKey_A;
        case GLFW_KEY_B: return ImGuiKey_B;
        case GLFW_KEY_C: return ImGuiKey_C;
        case GLFW_KEY_D: return ImGuiKey_D;
        case GLFW_KEY_E: return ImGuiKey_E;
        case GLFW_KEY_F: return ImGuiKey_F;
        case GLFW_KEY_G: return ImGuiKey_G;
        case GLFW_KEY_H: return ImGuiKey_H;
        case GLFW_KEY_I: return ImGuiKey_I;
        case GLFW_KEY_J: return ImGuiKey_J;
        case GLFW_KEY_K: return ImGuiKey_K;
        case GLFW_KEY_L: return ImGuiKey_L;
        case GLFW_KEY_M: return ImGuiKey_M;
        case GLFW_KEY_N: return ImGuiKey_N;
        case GLFW_KEY_O: return ImGuiKey_O;
        case GLFW_KEY_P: return ImGuiKey_P;
        case GLFW_KEY_Q: return ImGuiKey_Q;
        case GLFW_KEY_R: return ImGuiKey_R;
        case GLFW_KEY_S: return ImGuiKey_S;
        case GLFW_KEY_T: return ImGuiKey_T;
        case GLFW_KEY_U: return ImGuiKey_U;
        case GLFW_KEY_V: return ImGuiKey_V;
        case GLFW_KEY_W: return ImGuiKey_W;
        case GLFW_KEY_X: return ImGuiKey_X;
        case GLFW_KEY_Y: return ImGuiKey_Y;
        case GLFW_KEY_Z: return ImGuiKey_Z;
        case GLFW_KEY_F1: return ImGuiKey_F1;
        case GLFW_KEY_F2: return ImGuiKey_F2;
        case GLFW_KEY_F3: return ImGuiKey_F3;
        case GLFW_KEY_F4: return ImGuiKey_F4;
        case GLFW_KEY_F5: return ImGuiKey_F5;
        case GLFW_KEY_F6: return ImGuiKey_F6;
        case GLFW_KEY_F7: return ImGuiKey_F7;
        case GLFW_KEY_F8: return ImGuiKey_F8;
        case GLFW_KEY_F9: return ImGuiKey_F9;
        case GLFW_KEY_F10: return ImGuiKey_F10;
        case GLFW_KEY_F11: return ImGuiKey_F11;
        case GLFW_KEY_F12: return ImGuiKey_F12;
        case GLFW_KEY_F13: return ImGuiKey_F13;
        case GLFW_KEY_F14: return ImGuiKey_F14;
        case GLFW_KEY_F15: return ImGuiKey_F15;
        case GLFW_KEY_F16: return ImGuiKey_F16;
        case GLFW_KEY_F17: return ImGuiKey_F17;
        case GLFW_KEY_F18: return ImGuiKey_F18;
        case GLFW_KEY_F19: return ImGuiKey_F19;
        case GLFW_KEY_F20: return ImGuiKey_F20;
        case GLFW_KEY_F21: return ImGuiKey_F21;
        case GLFW_KEY_F22: return ImGuiKey_F22;
        case GLFW_KEY_F23: return ImGuiKey_F23;
        case GLFW_KEY_F24: return ImGuiKey_F24;
        default: return ImGuiKey_None;
    }
}

// X11 does not include current pressed/released modifier key in 'mods' flags submitted by GLFW
// See https://github.com/ocornut/imgui/issues/6034 and https://github.com/glfw/glfw/issues/1630
static void ImGui_ImplGlfw_UpdateKeyModifiers(GLFWwindow* window)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl,  (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Shift, (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT)   == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Alt,   (glfwGetKey(window, GLFW_KEY_LEFT_ALT)     == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_ALT)     == GLFW_PRESS));
    io.AddKeyEvent(ImGuiMod_Super, (glfwGetKey(window, GLFW_KEY_LEFT_SUPER)   == GLFW_PRESS) || (glfwGetKey(window, GLFW_KEY_RIGHT_SUPER)   == GLFW_PRESS));
}

static bool ImGui_ImplGlfw_ShouldChainCallback(GLFWwindow* window)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    return bd->CallbacksChainForAllWindows ? true : (window == bd->Window);
}

void ImGui_ImplGlfw_MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (g_PrevUserCallbackMousebutton != NULL && window == g_Window)
        g_PrevUserCallbackMousebutton(window, button, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    if (button >= 0 && button < ImGuiMouseButton_COUNT)
        io.AddMouseButtonEvent(button, action == GLFW_PRESS);
}

void ImGui_ImplGlfw_ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (g_PrevUserCallbackScroll != NULL && window == g_Window)
        g_PrevUserCallbackScroll(window, xoffset, yoffset);

void ImGui_ImplGlfw_KeyCallback(GLFWwindow* window, int keycode, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if (bd->PrevUserCallbackKey != nullptr && ImGui_ImplGlfw_ShouldChainCallback(window))
        bd->PrevUserCallbackKey(window, keycode, scancode, action, mods);

    if (action != GLFW_PRESS && action != GLFW_RELEASE)
        return;

    ImGui_ImplGlfw_UpdateKeyModifiers(window);

    keycode = ImGui_ImplGlfw_TranslateUntranslatedKey(keycode, scancode);

    ImGuiIO& io = ImGui::GetIO();
    ImGuiKey imgui_key = ImGui_ImplGlfw_KeyToImGuiKey(keycode);
    io.AddKeyEvent(imgui_key, (action == GLFW_PRESS));
    io.SetKeyEventNativeData(imgui_key, keycode, scancode); // To support legacy indexing (<1.87 user code)
}

void ImGui_ImplGlfw_WindowFocusCallback(GLFWwindow* window, int focused)
{
    if (g_PrevUserCallbackKey != NULL && window == g_Window)
        g_PrevUserCallbackKey(window, key, scancode, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    io.AddMousePosEvent((float)x, (float)y);
    bd->LastValidMousePos = ImVec2((float)x, (float)y);
}

// Workaround: X11 seems to send spurious Leave/Enter events which would make us lose our position,
// so we back it up and restore on Leave/Enter (see https://github.com/ocornut/imgui/issues/4984)
void ImGui_ImplGlfw_CursorEnterCallback(GLFWwindow* window, int entered)
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if (bd->PrevUserCallbackCursorEnter != nullptr && ImGui_ImplGlfw_ShouldChainCallback(window))
        bd->PrevUserCallbackCursorEnter(window, entered);

    ImGuiIO& io = ImGui::GetIO();
    if (entered)
    {
        bd->MouseWindow = window;
        io.AddMousePosEvent(bd->LastValidMousePos.x, bd->LastValidMousePos.y);
    }
    else if (!entered && bd->MouseWindow == window)
    {
        bd->LastValidMousePos = io.MousePos;
        bd->MouseWindow = nullptr;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }
}

void ImGui_ImplGlfw_CharCallback(GLFWwindow* window, unsigned int c)
{
    if (g_PrevUserCallbackChar != NULL && window == g_Window)
        g_PrevUserCallbackChar(window, c);

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void ImGui_ImplGlfw_MonitorCallback(GLFWmonitor*, int)
{
    g_WantUpdateMonitors = true;
}

static bool ImGui_ImplGlfw_Init(GLFWwindow* window, bool install_callbacks, GlfwClientApi client_api)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");
    //printf("GLFW_VERSION: %d.%d.%d (%d)", GLFW_VERSION_MAJOR, GLFW_VERSION_MINOR, GLFW_VERSION_REVISION, GLFW_VERSION_COMBINED);

    // Setup backend capabilities flags
    ImGui_ImplGlfw_Data* bd = IM_NEW(ImGui_ImplGlfw_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_glfw";
    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
    io.BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;    // We can create multi-viewports on the Platform side (optional)
#if GLFW_HAS_MOUSE_PASSTHROUGH || (GLFW_HAS_WINDOW_HOVERED && defined(_WIN32))
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport; // We can set io.MouseHoveredViewport correctly (optional, not easy)
#endif
    io.BackendPlatformName = "imgui_impl_glfw";

    bd->Window = window;
    bd->Time = 0.0;

    io.SetClipboardTextFn = ImGui_ImplGlfw_SetClipboardText;
    io.GetClipboardTextFn = ImGui_ImplGlfw_GetClipboardText;
    io.ClipboardUserData = g_Window;

    // Create mouse cursors
    // (By design, on X11 cursors are user configurable and some cursors may be missing. When a cursor doesn't exist,
    // GLFW will emit an error which will often be printed by the app, so we temporarily disable error reporting.
    // Missing cursors will return nullptr and our _UpdateMouseCursor() function will use the Arrow cursor instead.)
    GLFWerrorfun prev_error_callback = glfwSetErrorCallback(nullptr);
    bd->MouseCursors[ImGuiMouseCursor_Arrow] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_TextInput] = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = glfwCreateStandardCursor(GLFW_VRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = glfwCreateStandardCursor(GLFW_HRESIZE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_Hand] = glfwCreateStandardCursor(GLFW_HAND_CURSOR);
#if GLFW_HAS_NEW_CURSORS
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_RESIZE_ALL_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_RESIZE_NESW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_RESIZE_NWSE_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_NOT_ALLOWED_CURSOR);
#else
    bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
    bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = glfwCreateStandardCursor(GLFW_ARROW_CURSOR);
#endif
    glfwSetErrorCallback(prev_error_callback);
#if GLFW_HAS_GETERROR && !defined(__EMSCRIPTEN__) // Eat errors (see #5908)
    (void)glfwGetError(nullptr);
#endif

    // Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
    g_PrevUserCallbackMousebutton = NULL;
    g_PrevUserCallbackScroll = NULL;
    g_PrevUserCallbackKey = NULL;
    g_PrevUserCallbackChar = NULL;
    g_PrevUserCallbackMonitor = NULL;
    if (install_callbacks)
    {
        g_InstalledCallbacks = true;
        g_PrevUserCallbackMousebutton = glfwSetMouseButtonCallback(window, ImGui_ImplGlfw_MouseButtonCallback);
        g_PrevUserCallbackScroll = glfwSetScrollCallback(window, ImGui_ImplGlfw_ScrollCallback);
        g_PrevUserCallbackKey = glfwSetKeyCallback(window, ImGui_ImplGlfw_KeyCallback);
        g_PrevUserCallbackChar = glfwSetCharCallback(window, ImGui_ImplGlfw_CharCallback);
        g_PrevUserCallbackMonitor = glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);
    }

    // Update monitors the first time (note: monitor callback are broken in GLFW 3.2 and earlier, see github.com/glfw/glfw/issues/784)
    ImGui_ImplGlfw_UpdateMonitors();
    glfwSetMonitorCallback(ImGui_ImplGlfw_MonitorCallback);

    // Our mouse update function expect PlatformHandle to be filled for the main viewport
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    main_viewport->PlatformHandle = (void*)g_Window;
#ifdef _WIN32
    main_viewport->PlatformHandleRaw = glfwGetWin32Window(g_Window);
#endif
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        ImGui_ImplGlfw_InitPlatformInterface();

    g_ClientApi = client_api;
    return true;
}

bool ImGui_ImplGlfw_InitForOpenGL(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_OpenGL);
}

bool ImGui_ImplGlfw_InitForVulkan(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_Vulkan);
}

bool ImGui_ImplGlfw_InitForOther(GLFWwindow* window, bool install_callbacks)
{
    return ImGui_ImplGlfw_Init(window, install_callbacks, GlfwClientApi_Unknown);
}

void ImGui_ImplGlfw_Shutdown()
{
    ImGui_ImplGlfw_ShutdownPlatformInterface();

    if (g_InstalledCallbacks)
    {
        glfwSetMouseButtonCallback(g_Window, g_PrevUserCallbackMousebutton);
        glfwSetScrollCallback(g_Window, g_PrevUserCallbackScroll);
        glfwSetKeyCallback(g_Window, g_PrevUserCallbackKey);
        glfwSetCharCallback(g_Window, g_PrevUserCallbackChar);
        g_InstalledCallbacks = false;
    }

    for (ImGuiMouseCursor cursor_n = 0; cursor_n < ImGuiMouseCursor_COUNT; cursor_n++)
        glfwDestroyCursor(bd->MouseCursors[cursor_n]);

    // Windows: register a WndProc hook so we can intercept some messages.
#ifdef _WIN32
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ::SetWindowLongPtrW((HWND)main_viewport->PlatformHandleRaw, GWLP_WNDPROC, (LONG_PTR)bd->GlfwWndProc);
    bd->GlfwWndProc = nullptr;
#endif

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos | ImGuiBackendFlags_HasGamepad);
    IM_DELETE(bd);
}

static void ImGui_ImplGlfw_UpdateMouseData()
{
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    // (those braces are here to reduce diff with multi-viewports support in 'docking' branch)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        io.MouseDown[i] = g_MouseJustPressed[i] || glfwGetMouseButton(g_Window, i) != 0;
        g_MouseJustPressed[i] = false;
    }

    // Update mouse position
    const ImVec2 mouse_pos_backup = io.MousePos;
    io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
    io.MouseHoveredViewport = 0;
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        ImGuiViewport* viewport = platform_io.Viewports[n];
        GLFWwindow* window = (GLFWwindow*)viewport->PlatformHandle;
        IM_ASSERT(window != NULL);
#ifdef __EMSCRIPTEN__
        const bool focused = true;
        IM_ASSERT(platform_io.Viewports.Size == 1);
#else
        const bool focused = glfwGetWindowAttrib(window, GLFW_FOCUSED) != 0;
#endif
        if (focused)
        {
            if (io.WantSetMousePos)
            {
                glfwSetCursorPos(window, (double)(mouse_pos_backup.x - viewport->Pos.x), (double)(mouse_pos_backup.y - viewport->Pos.y));
            }
            else
            {
                double mouse_x, mouse_y;
                glfwGetCursorPos(window, &mouse_x, &mouse_y);
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    // Multi-viewport mode: mouse position in OS absolute coordinates (io.MousePos is (0,0) when the mouse is on the upper-left of the primary monitor)
                    int window_x, window_y;
                    glfwGetWindowPos(window, &window_x, &window_y);
                    io.MousePos = ImVec2((float)mouse_x + window_x, (float)mouse_y + window_y);
                }
                else
                {
                    // Single viewport mode: mouse position in client window coordinates (io.MousePos is (0,0) when the mouse is on the upper-left corner of the app window)
                    io.MousePos = ImVec2((float)mouse_x, (float)mouse_y);
                }
            }
            for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
                io.MouseDown[i] |= glfwGetMouseButton(window, i) != 0;
        }

        // (Optional) When using multiple viewports: set io.MouseHoveredViewport to the viewport the OS mouse cursor is hovering.
        // Important: this information is not easy to provide and many high-level windowing library won't be able to provide it correctly, because
        // - This is _ignoring_ viewports with the ImGuiViewportFlags_NoInputs flag (pass-through windows).
        // - This is _regardless_ of whether another viewport is focused or being dragged from.
        // If ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, imgui will ignore this field and infer the information by relying on the
        // rectangles and last focused time of every viewports it knows about. It will be unaware of other windows that may be sitting between or over your windows.
        // [GLFW] FIXME: This is currently only correct on Win32. See what we do below with the WM_NCHITTEST, missing an equivalent for other systems.
        // See https://github.com/glfw/glfw/issues/1236 if you want to help in making this a GLFW feature.
#if GLFW_HAS_MOUSE_PASSTHROUGH || (GLFW_HAS_WINDOW_HOVERED && defined(_WIN32))
        const bool window_no_input = (viewport->Flags & ImGuiViewportFlags_NoInputs) != 0;
#if GLFW_HAS_MOUSE_PASSTHROUGH
        glfwSetWindowAttrib(window, GLFW_MOUSE_PASSTHROUGH, window_no_input);
#endif
        if (glfwGetWindowAttrib(window, GLFW_HOVERED) && !window_no_input)
            io.MouseHoveredViewport = viewport->ID;
#endif
    }
}

static void ImGui_ImplGlfw_UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) || glfwGetInputMode(bd->Window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED)
        return;

    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (int n = 0; n < platform_io.Viewports.Size; n++)
    {
        GLFWwindow* window = (GLFWwindow*)platform_io.Viewports[n]->PlatformHandle;
        if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
        {
            // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
        else
        {
            // Show OS mouse cursor
            // FIXME-PLATFORM: Unfocused windows seems to fail changing the mouse cursor with GLFW 3.2, but 3.3 works here.
            glfwSetCursor(window, g_MouseCursors[imgui_cursor] ? g_MouseCursors[imgui_cursor] : g_MouseCursors[ImGuiMouseCursor_Arrow]);
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

// Update gamepad inputs
static inline float Saturate(float v) { return v < 0.0f ? 0.0f : v  > 1.0f ? 1.0f : v; }
static void ImGui_ImplGlfw_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0) // FIXME: Technically feeding gamepad shouldn't depend on this now that they are regular inputs.
        return;

    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
#if GLFW_HAS_GAMEPAD_API && !defined(__EMSCRIPTEN__)
    GLFWgamepadstate gamepad;
    if (!glfwGetGamepadState(GLFW_JOYSTICK_1, &gamepad))
        return;
    #define MAP_BUTTON(KEY_NO, BUTTON_NO, _UNUSED)          do { io.AddKeyEvent(KEY_NO, gamepad.buttons[BUTTON_NO] != 0); } while (0)
    #define MAP_ANALOG(KEY_NO, AXIS_NO, _UNUSED, V0, V1)    do { float v = gamepad.axes[AXIS_NO]; v = (v - V0) / (V1 - V0); io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); } while (0)
#else
    int axes_count = 0, buttons_count = 0;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
    const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
    if (axes_count == 0 || buttons_count == 0)
        return;
    #define MAP_BUTTON(KEY_NO, _UNUSED, BUTTON_NO)          do { io.AddKeyEvent(KEY_NO, (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS)); } while (0)
    #define MAP_ANALOG(KEY_NO, _UNUSED, AXIS_NO, V0, V1)    do { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); io.AddKeyAnalogEvent(KEY_NO, v > 0.10f, Saturate(v)); } while (0)
#endif
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    MAP_BUTTON(ImGuiKey_GamepadStart,       GLFW_GAMEPAD_BUTTON_START,          7);
    MAP_BUTTON(ImGuiKey_GamepadBack,        GLFW_GAMEPAD_BUTTON_BACK,           6);
    MAP_BUTTON(ImGuiKey_GamepadFaceLeft,    GLFW_GAMEPAD_BUTTON_X,              2);     // Xbox X, PS Square
    MAP_BUTTON(ImGuiKey_GamepadFaceRight,   GLFW_GAMEPAD_BUTTON_B,              1);     // Xbox B, PS Circle
    MAP_BUTTON(ImGuiKey_GamepadFaceUp,      GLFW_GAMEPAD_BUTTON_Y,              3);     // Xbox Y, PS Triangle
    MAP_BUTTON(ImGuiKey_GamepadFaceDown,    GLFW_GAMEPAD_BUTTON_A,              0);     // Xbox A, PS Cross
    MAP_BUTTON(ImGuiKey_GamepadDpadLeft,    GLFW_GAMEPAD_BUTTON_DPAD_LEFT,      13);
    MAP_BUTTON(ImGuiKey_GamepadDpadRight,   GLFW_GAMEPAD_BUTTON_DPAD_RIGHT,     11);
    MAP_BUTTON(ImGuiKey_GamepadDpadUp,      GLFW_GAMEPAD_BUTTON_DPAD_UP,        10);
    MAP_BUTTON(ImGuiKey_GamepadDpadDown,    GLFW_GAMEPAD_BUTTON_DPAD_DOWN,      12);
    MAP_BUTTON(ImGuiKey_GamepadL1,          GLFW_GAMEPAD_BUTTON_LEFT_BUMPER,    4);
    MAP_BUTTON(ImGuiKey_GamepadR1,          GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER,   5);
    MAP_ANALOG(ImGuiKey_GamepadL2,          GLFW_GAMEPAD_AXIS_LEFT_TRIGGER,     4,      -0.75f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadR2,          GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER,    5,      -0.75f,  +1.0f);
    MAP_BUTTON(ImGuiKey_GamepadL3,          GLFW_GAMEPAD_BUTTON_LEFT_THUMB,     8);
    MAP_BUTTON(ImGuiKey_GamepadR3,          GLFW_GAMEPAD_BUTTON_RIGHT_THUMB,    9);
    MAP_ANALOG(ImGuiKey_GamepadLStickLeft,  GLFW_GAMEPAD_AXIS_LEFT_X,           0,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickRight, GLFW_GAMEPAD_AXIS_LEFT_X,           0,      +0.25f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickUp,    GLFW_GAMEPAD_AXIS_LEFT_Y,           1,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadLStickDown,  GLFW_GAMEPAD_AXIS_LEFT_Y,           1,      +0.25f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickLeft,  GLFW_GAMEPAD_AXIS_RIGHT_X,          2,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickRight, GLFW_GAMEPAD_AXIS_RIGHT_X,          2,      +0.25f,  +1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickUp,    GLFW_GAMEPAD_AXIS_RIGHT_Y,          3,      -0.25f,  -1.0f);
    MAP_ANALOG(ImGuiKey_GamepadRStickDown,  GLFW_GAMEPAD_AXIS_RIGHT_Y,          3,      +0.25f,  +1.0f);
    #undef MAP_BUTTON
    #undef MAP_ANALOG
}

static void ImGui_ImplGlfw_UpdateMonitors()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    int monitors_count = 0;
    GLFWmonitor** glfw_monitors = glfwGetMonitors(&monitors_count);
    platform_io.Monitors.resize(0);
    for (int n = 0; n < monitors_count; n++)
    {
        ImGuiPlatformMonitor monitor;
        int x, y;
        glfwGetMonitorPos(glfw_monitors[n], &x, &y);
        const GLFWvidmode* vid_mode = glfwGetVideoMode(glfw_monitors[n]);
        monitor.MainPos = monitor.WorkPos = ImVec2((float)x, (float)y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float)vid_mode->width, (float)vid_mode->height);
#if GLFW_HAS_MONITOR_WORK_AREA
        int w, h;
        glfwGetMonitorWorkarea(glfw_monitors[n], &x, &y, &w, &h);
        if (w > 0 && h > 0) // Workaround a small GLFW issue reporting zero on monitor changes: https://github.com/glfw/glfw/pull/1761
        {
            monitor.WorkPos = ImVec2((float)x, (float)y);
            monitor.WorkSize = ImVec2((float)w, (float)h);
        }
#endif
#if GLFW_HAS_PER_MONITOR_DPI
        // Warning: the validity of monitor DPI information on Windows depends on the application DPI awareness settings, which generally needs to be set in the manifest or at runtime.
        float x_scale, y_scale;
        glfwGetMonitorContentScale(glfw_monitors[n], &x_scale, &y_scale);
        monitor.DpiScale = x_scale;
#endif
        platform_io.Monitors.push_back(monitor);
    }
    g_WantUpdateMonitors = false;
}

void ImGui_ImplGlfw_NewFrame()
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplGlfw_Data* bd = ImGui_ImplGlfw_GetBackendData();
    IM_ASSERT(bd != nullptr && "Did you call ImGui_ImplGlfw_InitForXXX()?");

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int display_w, display_h;
    glfwGetWindowSize(bd->Window, &w, &h);
    glfwGetFramebufferSize(bd->Window, &display_w, &display_h);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale = ImVec2((float)display_w / w, (float)display_h / h);
    if (g_WantUpdateMonitors)
        ImGui_ImplGlfw_UpdateMonitors();

    // Setup time step
    // (Accept glfwGetTime() not returning a monotonically increasing value. Seems to happens on disconnecting peripherals and probably on VMs and Emscripten, see #6491, #6189, #6114, #3644)
    double current_time = glfwGetTime();
    if (current_time <= bd->Time)
        current_time = bd->Time + 0.00001f;
    io.DeltaTime = bd->Time > 0.0 ? (float)(current_time - bd->Time) : (float)(1.0f / 60.0f);
    bd->Time = current_time;

    ImGui_ImplGlfw_UpdateMouseData();
    ImGui_ImplGlfw_UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGui_ImplGlfw_UpdateGamepads();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create and handle multiple viewports simultaneously.
// If you are new to dear imgui or creating a new binding for dear imgui, it is recommended that you completely ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RenderUserData field of each ImGuiViewport to easily retrieve our backend data.
struct ImGuiViewportDataGlfw
{
    GLFWwindow* Window;
    bool        WindowOwned;
    int         IgnoreWindowPosEventFrame;
    int         IgnoreWindowSizeEventFrame;

    ImGuiViewportDataGlfw()  { Window = NULL; WindowOwned = false; IgnoreWindowSizeEventFrame = IgnoreWindowPosEventFrame = -1; }
    ~ImGuiViewportDataGlfw() { IM_ASSERT(Window == NULL); }
};

static void ImGui_ImplGlfw_WindowCloseCallback(GLFWwindow* window)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
        viewport->PlatformRequestClose = true;
}

// GLFW may dispatch window pos/size events after calling glfwSetWindowPos()/glfwSetWindowSize().
// However: depending on the platform the callback may be invoked at different time:
// - on Windows it appears to be called within the glfwSetWindowPos()/glfwSetWindowSize() call
// - on Linux it is queued and invoked during glfwPollEvents()
// Because the event doesn't always fire on glfwSetWindowXXX() we use a frame counter tag to only
// ignore recent glfwSetWindowXXX() calls.
static void ImGui_ImplGlfw_WindowPosCallback(GLFWwindow* window, int, int)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData)
        {
            bool ignore_event = (ImGui::GetFrameCount() <= data->IgnoreWindowPosEventFrame + 1);
            //data->IgnoreWindowPosEventFrame = -1;
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestMove = true;
    }
}

static void ImGui_ImplGlfw_WindowSizeCallback(GLFWwindow* window, int, int)
{
    if (ImGuiViewport* viewport = ImGui::FindViewportByPlatformHandle(window))
    {
        if (ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData)
        {
            bool ignore_event = (ImGui::GetFrameCount() <= data->IgnoreWindowSizeEventFrame + 1);
            //data->IgnoreWindowSizeEventFrame = -1;
            if (ignore_event)
                return;
        }
        viewport->PlatformRequestResize = true;
    }
}

static void ImGui_ImplGlfw_CreateWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataGlfw* data = IM_NEW(ImGuiViewportDataGlfw)();
    viewport->PlatformUserData = data;

    // GLFW 3.2 unfortunately always set focus on glfwCreateWindow() if GLFW_VISIBLE is set, regardless of GLFW_FOCUSED
    // With GLFW 3.3, the hint GLFW_FOCUS_ON_SHOW fixes this problem
    glfwWindowHint(GLFW_VISIBLE, false);
    glfwWindowHint(GLFW_FOCUSED, false);
#if GLFW_HAS_FOCUS_ON_SHOW
     glfwWindowHint(GLFW_FOCUS_ON_SHOW, false);
 #endif
    glfwWindowHint(GLFW_DECORATED, (viewport->Flags & ImGuiViewportFlags_NoDecoration) ? false : true);
#if GLFW_HAS_WINDOW_TOPMOST
    glfwWindowHint(GLFW_FLOATING, (viewport->Flags & ImGuiViewportFlags_TopMost) ? true : false);
#endif
    GLFWwindow* share_window = (g_ClientApi == GlfwClientApi_OpenGL) ? g_Window : NULL;
    data->Window = glfwCreateWindow((int)viewport->Size.x, (int)viewport->Size.y, "No Title Yet", NULL, share_window);
    data->WindowOwned = true;
    viewport->PlatformHandle = (void*)data->Window;
#ifdef _WIN32
    viewport->PlatformHandleRaw = glfwGetWin32Window(data->Window);
#endif
    glfwSetWindowPos(data->Window, (int)viewport->Pos.x, (int)viewport->Pos.y);

    // Install GLFW callbacks for secondary viewports
    glfwSetMouseButtonCallback(data->Window, ImGui_ImplGlfw_MouseButtonCallback);
    glfwSetScrollCallback(data->Window, ImGui_ImplGlfw_ScrollCallback);
    glfwSetKeyCallback(data->Window, ImGui_ImplGlfw_KeyCallback);
    glfwSetCharCallback(data->Window, ImGui_ImplGlfw_CharCallback);
    glfwSetWindowCloseCallback(data->Window, ImGui_ImplGlfw_WindowCloseCallback);
    glfwSetWindowPosCallback(data->Window, ImGui_ImplGlfw_WindowPosCallback);
    glfwSetWindowSizeCallback(data->Window, ImGui_ImplGlfw_WindowSizeCallback);
    if (g_ClientApi == GlfwClientApi_OpenGL)
    {
        glfwMakeContextCurrent(data->Window);
        glfwSwapInterval(0);
    }
}

static void ImGui_ImplGlfw_DestroyWindow(ImGuiViewport* viewport)
{
    if (ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData)
    {
        if (data->WindowOwned)
        {
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED && defined(_WIN32)
            HWND hwnd = (HWND)viewport->PlatformHandleRaw;
            ::RemovePropA(hwnd, "IMGUI_VIEWPORT");
#endif
            glfwDestroyWindow(data->Window);
        }
        data->Window = NULL;
        IM_DELETE(data);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = NULL;
}

// We have submitted https://github.com/glfw/glfw/pull/1568 to allow GLFW to support "transparent inputs".
// In the meanwhile we implement custom per-platform workarounds here (FIXME-VIEWPORT: Implement same work-around for Linux/OSX!)
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED && defined(_WIN32)
static WNDPROC g_GlfwWndProc = NULL;
static LRESULT CALLBACK WndProcNoInputs(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_NCHITTEST)
    {
        // Let mouse pass-through the window. This will allow the backend to set io.MouseHoveredViewport properly (which is OPTIONAL).
        // The ImGuiViewportFlags_NoInputs flag is set while dragging a viewport, as want to detect the window behind the one we are dragging.
        // If you cannot easily access those viewport flags from your windowing/event code: you may manually synchronize its state e.g. in
        // your main loop after calling UpdatePlatformWindows(). Iterate all viewports/platform windows and pass the flag to your windowing system.
        ImGuiViewport* viewport = (ImGuiViewport*)::GetPropA(hWnd, "IMGUI_VIEWPORT");
        if (viewport->Flags & ImGuiViewportFlags_NoInputs)
            return HTTRANSPARENT;
    }
    return ::CallWindowProc(g_GlfwWndProc, hWnd, msg, wParam, lParam);
}
#endif

static void ImGui_ImplGlfw_ShowWindow(ImGuiViewport* viewport)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;

#if defined(_WIN32)
    // GLFW hack: Hide icon from task bar
    HWND hwnd = (HWND)viewport->PlatformHandleRaw;
    if (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
    {
        LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ex_style &= ~WS_EX_APPWINDOW;
        ex_style |= WS_EX_TOOLWINDOW;
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }

    // GLFW hack: install hook for WM_NCHITTEST message handler
#if !GLFW_HAS_MOUSE_PASSTHROUGH && GLFW_HAS_WINDOW_HOVERED && defined(_WIN32)
    ::SetPropA(hwnd, "IMGUI_VIEWPORT", viewport);
    if (g_GlfwWndProc == NULL)
        g_GlfwWndProc = (WNDPROC)::GetWindowLongPtr(hwnd, GWLP_WNDPROC);
    ::SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)WndProcNoInputs);
#endif

#if !GLFW_HAS_FOCUS_ON_SHOW
    // GLFW hack: GLFW 3.2 has a bug where glfwShowWindow() also activates/focus the window.
    // The fix was pushed to GLFW repository on 2018/01/09 and should be included in GLFW 3.3 via a GLFW_FOCUS_ON_SHOW window attribute.
    // See https://github.com/glfw/glfw/issues/1189
    // FIXME-VIEWPORT: Implement same work-around for Linux/OSX in the meanwhile.
    if (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing)
    {
        ::ShowWindow(hwnd, SW_SHOWNA);
        return;
    }
#endif
#endif

    glfwShowWindow(data->Window);
}

static ImVec2 ImGui_ImplGlfw_GetWindowPos(ImGuiViewport* viewport)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    int x = 0, y = 0;
    glfwGetWindowPos(data->Window, &x, &y);
    return ImVec2((float)x, (float)y);
}

static void ImGui_ImplGlfw_SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    data->IgnoreWindowPosEventFrame = ImGui::GetFrameCount();
    glfwSetWindowPos(data->Window, (int)pos.x, (int)pos.y);
}

static ImVec2 ImGui_ImplGlfw_GetWindowSize(ImGuiViewport* viewport)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    int w = 0, h = 0;
    glfwGetWindowSize(data->Window, &w, &h);
    return ImVec2((float)w, (float)h);
}

static void ImGui_ImplGlfw_SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
#if __APPLE__ && !GLFW_HAS_OSX_WINDOW_POS_FIX
    // Native OS windows are positioned from the bottom-left corner on macOS, whereas on other platforms they are
    // positioned from the upper-left corner. GLFW makes an effort to convert macOS style coordinates, however it
    // doesn't handle it when changing size. We are manually moving the window in order for changes of size to be based
    // on the upper-left corner.
    int x, y, width, height;
    glfwGetWindowPos(data->Window, &x, &y);
    glfwGetWindowSize(data->Window, &width, &height);
    glfwSetWindowPos(data->Window, x, y - height + size.y);
#endif
    data->IgnoreWindowSizeEventFrame = ImGui::GetFrameCount();
    glfwSetWindowSize(data->Window, (int)size.x, (int)size.y);
}

static void ImGui_ImplGlfw_SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    glfwSetWindowTitle(data->Window, title);
}

static void ImGui_ImplGlfw_SetWindowFocus(ImGuiViewport* viewport)
{
#if GLFW_HAS_FOCUS_WINDOW
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    glfwFocusWindow(data->Window);
#else
    // FIXME: What are the effect of not having this function? At the moment imgui doesn't actually call SetWindowFocus - we set that up ahead, will answer that question later.
    (void)viewport;
#endif
}

static bool ImGui_ImplGlfw_GetWindowFocus(ImGuiViewport* viewport)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    return glfwGetWindowAttrib(data->Window, GLFW_FOCUSED) != 0;
}

static bool ImGui_ImplGlfw_GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    return glfwGetWindowAttrib(data->Window, GLFW_ICONIFIED) != 0;
}

#if GLFW_HAS_WINDOW_ALPHA
static void ImGui_ImplGlfw_SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    glfwSetWindowOpacity(data->Window, alpha);
}
#endif

static void ImGui_ImplGlfw_RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    if (g_ClientApi == GlfwClientApi_OpenGL)
        glfwMakeContextCurrent(data->Window);
}

static void ImGui_ImplGlfw_SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    if (g_ClientApi == GlfwClientApi_OpenGL)
    {
        glfwMakeContextCurrent(data->Window);
        glfwSwapBuffers(data->Window);
    }
}

//--------------------------------------------------------------------------------------------------------
// IME (Input Method Editor) basic support for e.g. Asian language users
//--------------------------------------------------------------------------------------------------------

// We provide a Win32 implementation because this is such a common issue for IME users
#if defined(_WIN32) && !defined(IMGUI_DISABLE_WIN32_FUNCTIONS) && !defined(IMGUI_DISABLE_WIN32_DEFAULT_IME_FUNCTIONS) && !defined(__GNUC__)
#define HAS_WIN32_IME   1
#include <imm.h>
#ifdef _MSC_VER
#pragma comment(lib, "imm32")
#endif
static void ImGui_ImplWin32_SetImeInputPos(ImGuiViewport* viewport, ImVec2 pos)
{
    COMPOSITIONFORM cf = { CFS_FORCE_POSITION, { (LONG)(pos.x - viewport->Pos.x), (LONG)(pos.y - viewport->Pos.y) }, { 0, 0, 0, 0 } };
    if (HWND hwnd = (HWND)viewport->PlatformHandleRaw)
        if (HIMC himc = ::ImmGetContext(hwnd))
        {
            ::ImmSetCompositionWindow(himc, &cf);
            ::ImmReleaseContext(hwnd, himc);
        }
}
#else
#define HAS_WIN32_IME   0
#endif

//--------------------------------------------------------------------------------------------------------
// Vulkan support (the Vulkan renderer needs to call a platform-side support function to create the surface)
//--------------------------------------------------------------------------------------------------------

// Avoid including <vulkan.h> so we can build without it
#if GLFW_HAS_VULKAN
#ifndef VULKAN_H_
#define VK_DEFINE_HANDLE(object) typedef struct object##_T* object;
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || defined(_M_X64) || defined(__ia64) || defined (_M_IA64) || defined(__aarch64__) || defined(__powerpc64__)
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef struct object##_T *object;
#else
#define VK_DEFINE_NON_DISPATCHABLE_HANDLE(object) typedef uint64_t object;
#endif
VK_DEFINE_HANDLE(VkInstance)
VK_DEFINE_NON_DISPATCHABLE_HANDLE(VkSurfaceKHR)
struct VkAllocationCallbacks;
enum VkResult { VK_RESULT_MAX_ENUM = 0x7FFFFFFF };
#endif // VULKAN_H_
extern "C" { extern GLFWAPI VkResult glfwCreateWindowSurface(VkInstance instance, GLFWwindow* window, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface); }
static int ImGui_ImplGlfw_CreateVkSurface(ImGuiViewport* viewport, ImU64 vk_instance, const void* vk_allocator, ImU64* out_vk_surface)
{
    ImGuiViewportDataGlfw* data = (ImGuiViewportDataGlfw*)viewport->PlatformUserData;
    IM_ASSERT(g_ClientApi == GlfwClientApi_Vulkan);
    VkResult err = glfwCreateWindowSurface((VkInstance)vk_instance, data->Window, (const VkAllocationCallbacks*)vk_allocator, (VkSurfaceKHR*)out_vk_surface);
    return (int)err;
}
#endif // GLFW_HAS_VULKAN

static void ImGui_ImplGlfw_InitPlatformInterface()
{
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    platform_io.Platform_CreateWindow = ImGui_ImplGlfw_CreateWindow;
    platform_io.Platform_DestroyWindow = ImGui_ImplGlfw_DestroyWindow;
    platform_io.Platform_ShowWindow = ImGui_ImplGlfw_ShowWindow;
    platform_io.Platform_SetWindowPos = ImGui_ImplGlfw_SetWindowPos;
    platform_io.Platform_GetWindowPos = ImGui_ImplGlfw_GetWindowPos;
    platform_io.Platform_SetWindowSize = ImGui_ImplGlfw_SetWindowSize;
    platform_io.Platform_GetWindowSize = ImGui_ImplGlfw_GetWindowSize;
    platform_io.Platform_SetWindowFocus = ImGui_ImplGlfw_SetWindowFocus;
    platform_io.Platform_GetWindowFocus = ImGui_ImplGlfw_GetWindowFocus;
    platform_io.Platform_GetWindowMinimized = ImGui_ImplGlfw_GetWindowMinimized;
    platform_io.Platform_SetWindowTitle = ImGui_ImplGlfw_SetWindowTitle;
    platform_io.Platform_RenderWindow = ImGui_ImplGlfw_RenderWindow;
    platform_io.Platform_SwapBuffers = ImGui_ImplGlfw_SwapBuffers;
#if GLFW_HAS_WINDOW_ALPHA
    platform_io.Platform_SetWindowAlpha = ImGui_ImplGlfw_SetWindowAlpha;
#endif
#if GLFW_HAS_VULKAN
    platform_io.Platform_CreateVkSurface = ImGui_ImplGlfw_CreateVkSurface;
#endif
#if HAS_WIN32_IME
    platform_io.Platform_SetImeInputPos = ImGui_ImplWin32_SetImeInputPos;
#endif

    // Register main window handle (which is owned by the main application, not by us)
    // This is mostly for simplicity and consistency, so that our code (e.g. mouse handling etc.) can use same logic for main and secondary viewports.
    ImGuiViewport* main_viewport = ImGui::GetMainViewport();
    ImGuiViewportDataGlfw* data = IM_NEW(ImGuiViewportDataGlfw)();
    data->Window = g_Window;
    data->WindowOwned = false;
    main_viewport->PlatformUserData = data;
    main_viewport->PlatformHandle = (void*)g_Window;
}

static void ImGui_ImplGlfw_ShutdownPlatformInterface()
{
}

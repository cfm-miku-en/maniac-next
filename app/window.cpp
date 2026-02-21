#include "window.h"
#include "font.h"

#include <d3d9.h>
#include <tchar.h>
#include <stdexcept>
#include <imgui/backends/imgui_impl_dx9.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <random>
#include <algorithm>

#include <maniac/common.h>
#include <maniac/maniac.h>

static LPDIRECT3D9           g_pD3D       = NULL;
static LPDIRECT3DDEVICE9     g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS g_d3dpp      = {};


bool CreateDeviceD3D(HWND hWnd) {
    if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
        return false;

    ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
    g_d3dpp.Windowed               = TRUE;
    g_d3dpp.SwapEffect             = D3DSWAPEFFECT_DISCARD;
    g_d3dpp.BackBufferFormat       = D3DFMT_UNKNOWN;
    g_d3dpp.EnableAutoDepthStencil = TRUE;
    g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
    g_d3dpp.PresentationInterval   = D3DPRESENT_INTERVAL_ONE;

    if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
        return false;
    return true;
}

void CleanupDeviceD3D() {
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
    if (g_pD3D)       { g_pD3D->Release();       g_pD3D       = NULL; }
}

void ResetDevice() {
    ImGui_ImplDX9_InvalidateDeviceObjects();
    HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
    if (hr == D3DERR_INVALIDCALL) IM_ASSERT(0);
    ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
        case WM_SIZE:
            if (g_pd3dDevice && wParam != SIZE_MINIMIZED) {
                g_d3dpp.BackBufferWidth  = LOWORD(lParam);
                g_d3dpp.BackBufferHeight = HIWORD(lParam);
                ResetDevice();
            }
            return 0;
        case WM_SYSCOMMAND:
            if ((wParam & 0xfff0) == SC_KEYMENU) return 0;
            break;
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}


static std::string generate_random_string(int length) {
    std::string s;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> d(0, 61);
    for (int i = 0; i < length; ++i) {
        int r = d(gen);
        s += (r < 10) ? ('0' + r) : (r < 36) ? ('A' + r - 10) : ('a' + r - 36);
    }
    return s;
}

static void randomize_window_title(HWND window) {
    SetWindowTextA(window, generate_random_string(16).c_str());
}


namespace theme {

static void apply_cherry(float* accent) {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    ImVec4 bg       = ImVec4(0.10f, 0.08f, 0.10f, 1.00f);
    ImVec4 bgMid    = ImVec4(0.17f, 0.14f, 0.17f, 1.00f);
    ImVec4 bgHi     = ImVec4(0.22f, 0.18f, 0.22f, 1.00f);
    ImVec4 acc      = ImVec4(accent[0], accent[1], accent[2], 1.00f);
    ImVec4 accDim   = ImVec4(accent[0]*0.6f, accent[1]*0.6f, accent[2]*0.6f, 0.80f);
    ImVec4 accHover = ImVec4(accent[0]*0.8f, accent[1]*0.8f, accent[2]*0.8f, 1.00f);

    s.Colors[ImGuiCol_Text]                 = ImVec4(0.86f, 0.82f, 0.86f, 1.00f);
    s.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.50f, 0.45f, 0.50f, 1.00f);
    s.Colors[ImGuiCol_WindowBg]             = bg;
    s.Colors[ImGuiCol_ChildBg]              = bgMid;
    s.Colors[ImGuiCol_PopupBg]              = bgMid;
    s.Colors[ImGuiCol_Border]               = ImVec4(accent[0]*0.3f, accent[1]*0.3f, accent[2]*0.3f, 0.50f);
    s.Colors[ImGuiCol_BorderShadow]         = ImVec4(0,0,0,0);
    s.Colors[ImGuiCol_FrameBg]              = bgHi;
    s.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(bgHi.x+0.05f, bgHi.y+0.05f, bgHi.z+0.05f, 1.f);
    s.Colors[ImGuiCol_FrameBgActive]        = accDim;
    s.Colors[ImGuiCol_TitleBg]              = bg;
    s.Colors[ImGuiCol_TitleBgActive]        = bgMid;
    s.Colors[ImGuiCol_TitleBgCollapsed]     = bg;
    s.Colors[ImGuiCol_ScrollbarBg]          = bg;
    s.Colors[ImGuiCol_ScrollbarGrab]        = accDim;
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = accHover;
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = acc;
    s.Colors[ImGuiCol_CheckMark]            = acc;
    s.Colors[ImGuiCol_SliderGrab]           = acc;
    s.Colors[ImGuiCol_SliderGrabActive]     = accHover;
    s.Colors[ImGuiCol_Button]               = accDim;
    s.Colors[ImGuiCol_ButtonHovered]        = accHover;
    s.Colors[ImGuiCol_ButtonActive]         = acc;
    s.Colors[ImGuiCol_Header]               = ImVec4(accent[0]*0.4f, accent[1]*0.4f, accent[2]*0.4f, 0.45f);
    s.Colors[ImGuiCol_HeaderHovered]        = accHover;
    s.Colors[ImGuiCol_HeaderActive]         = acc;
    s.Colors[ImGuiCol_Separator]            = ImVec4(accent[0]*0.3f, accent[1]*0.3f, accent[2]*0.3f, 0.50f);
    s.Colors[ImGuiCol_SeparatorHovered]     = accHover;
    s.Colors[ImGuiCol_SeparatorActive]      = acc;
    s.Colors[ImGuiCol_ResizeGrip]           = accDim;
    s.Colors[ImGuiCol_ResizeGripHovered]    = accHover;
    s.Colors[ImGuiCol_ResizeGripActive]     = acc;
    s.Colors[ImGuiCol_Tab]                  = bgHi;
    s.Colors[ImGuiCol_TabHovered]           = accHover;
    s.Colors[ImGuiCol_TabSelected]          = acc;
    s.Colors[ImGuiCol_TabDimmed]            = bgMid;
    s.Colors[ImGuiCol_TabDimmedSelected]    = accDim;
    s.Colors[ImGuiCol_PlotLines]            = acc;
    s.Colors[ImGuiCol_PlotLinesHovered]     = accHover;
    s.Colors[ImGuiCol_PlotHistogram]        = acc;
    s.Colors[ImGuiCol_PlotHistogramHovered] = accHover;
    s.Colors[ImGuiCol_TableHeaderBg]        = bgMid;

    s.WindowRounding   = 5.0f;
    s.ChildRounding    = 5.0f;
    s.FrameRounding    = 4.0f;
    s.GrabRounding     = 4.0f;
    s.PopupRounding    = 4.0f;
    s.ScrollbarRounding= 6.0f;
    s.TabRounding      = 4.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize  = 1.0f;
    s.WindowPadding    = ImVec2(10, 10);
    s.FramePadding     = ImVec2(6,  4);
    s.ItemSpacing      = ImVec2(8,  6);
    s.ItemInnerSpacing = ImVec2(4,  4);
    s.GrabMinSize      = 10.0f;
    s.ScrollbarSize    = 12.0f;
}

static void apply_moonlight(float* accent) {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    ImVec4 bg       = ImVec4(0.113f, 0.129f, 0.180f, 1.00f);
    ImVec4 bgMid    = ImVec4(0.141f, 0.161f, 0.220f, 1.00f);
    ImVec4 bgHi     = ImVec4(0.172f, 0.196f, 0.259f, 1.00f);
    ImVec4 acc      = ImVec4(accent[0], accent[1], accent[2], 1.00f);
    ImVec4 accDim   = ImVec4(accent[0]*0.55f, accent[1]*0.55f, accent[2]*0.55f, 0.85f);
    ImVec4 accHover = ImVec4(accent[0]*0.80f, accent[1]*0.80f, accent[2]*0.80f, 1.00f);

    s.Colors[ImGuiCol_Text]                 = ImVec4(0.82f, 0.85f, 0.95f, 1.00f);
    s.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.45f, 0.50f, 0.65f, 1.00f);
    s.Colors[ImGuiCol_WindowBg]             = bg;
    s.Colors[ImGuiCol_ChildBg]              = bgMid;
    s.Colors[ImGuiCol_PopupBg]              = bgMid;
    s.Colors[ImGuiCol_Border]               = ImVec4(0.20f, 0.23f, 0.35f, 0.80f);
    s.Colors[ImGuiCol_BorderShadow]         = ImVec4(0,0,0,0);
    s.Colors[ImGuiCol_FrameBg]              = bgHi;
    s.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(bgHi.x+0.04f, bgHi.y+0.04f, bgHi.z+0.06f, 1.f);
    s.Colors[ImGuiCol_FrameBgActive]        = accDim;
    s.Colors[ImGuiCol_TitleBg]              = bg;
    s.Colors[ImGuiCol_TitleBgActive]        = bgMid;
    s.Colors[ImGuiCol_TitleBgCollapsed]     = bg;
    s.Colors[ImGuiCol_ScrollbarBg]          = bg;
    s.Colors[ImGuiCol_ScrollbarGrab]        = accDim;
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = accHover;
    s.Colors[ImGuiCol_ScrollbarGrabActive]  = acc;
    s.Colors[ImGuiCol_CheckMark]            = acc;
    s.Colors[ImGuiCol_SliderGrab]           = acc;
    s.Colors[ImGuiCol_SliderGrabActive]     = accHover;
    s.Colors[ImGuiCol_Button]               = accDim;
    s.Colors[ImGuiCol_ButtonHovered]        = accHover;
    s.Colors[ImGuiCol_ButtonActive]         = acc;
    s.Colors[ImGuiCol_Header]               = ImVec4(accent[0]*0.35f, accent[1]*0.35f, accent[2]*0.35f, 0.50f);
    s.Colors[ImGuiCol_HeaderHovered]        = accHover;
    s.Colors[ImGuiCol_HeaderActive]         = acc;
    s.Colors[ImGuiCol_Separator]            = ImVec4(0.20f, 0.23f, 0.35f, 1.00f);
    s.Colors[ImGuiCol_SeparatorHovered]     = accHover;
    s.Colors[ImGuiCol_SeparatorActive]      = acc;
    s.Colors[ImGuiCol_ResizeGrip]           = accDim;
    s.Colors[ImGuiCol_ResizeGripHovered]    = accHover;
    s.Colors[ImGuiCol_ResizeGripActive]     = acc;
    s.Colors[ImGuiCol_Tab]                  = bgHi;
    s.Colors[ImGuiCol_TabHovered]           = accHover;
    s.Colors[ImGuiCol_TabSelected]          = acc;
    s.Colors[ImGuiCol_TabDimmed]            = bgMid;
    s.Colors[ImGuiCol_TabDimmedSelected]    = accDim;
    s.Colors[ImGuiCol_PlotLines]            = acc;
    s.Colors[ImGuiCol_PlotLinesHovered]     = accHover;
    s.Colors[ImGuiCol_PlotHistogram]        = acc;
    s.Colors[ImGuiCol_PlotHistogramHovered] = accHover;
    s.Colors[ImGuiCol_TableHeaderBg]        = bgMid;

    s.WindowRounding   = 8.0f;
    s.ChildRounding    = 6.0f;
    s.FrameRounding    = 5.0f;
    s.GrabRounding     = 5.0f;
    s.PopupRounding    = 5.0f;
    s.ScrollbarRounding= 8.0f;
    s.TabRounding      = 5.0f;
    s.WindowBorderSize = 1.0f;
    s.FrameBorderSize  = 0.0f;
    s.WindowPadding    = ImVec2(12, 12);
    s.FramePadding     = ImVec2(8,  5);
    s.ItemSpacing      = ImVec2(8,  7);
    s.ItemInnerSpacing = ImVec2(5,  5);
    s.GrabMinSize      = 10.0f;
    s.ScrollbarSize    = 10.0f;
}

void apply(int theme_index, float* accent) {
    switch (theme_index) {
        case 0:  apply_cherry(accent);    break;
        case 1:  apply_moonlight(accent); break;
        default: apply_moonlight(accent); break;
    }
}

} 


void window::start(const std::function<void()>& body) {
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEX wc = {
        sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L,
        GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("maniac"), NULL
    };
    ::RegisterClassEx(&wc);

    HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("maniac"),
        WS_OVERLAPPEDWINDOW, 100, 100, 570, 490,
        NULL, NULL, wc.hInstance, NULL);

    randomize_window_title(hwnd);

    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        throw std::runtime_error("could not create d3d device");
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    theme::apply(maniac::config.theme_index, maniac::config.accent_color);

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(g_pd3dDevice);

    io.Fonts->AddFontFromMemoryCompressedTTF(
        Karla_compressed_data, Karla_compressed_size, 16.0f);

    int   last_theme  = maniac::config.theme_index;
    float last_acc[3] = {
        maniac::config.accent_color[0],
        maniac::config.accent_color[1],
        maniac::config.accent_color[2]
    };

    bool done = false;
    while (!done) {
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) done = true;
        }
        if (done) break;

        float* acc = maniac::config.accent_color;
        bool accent_changed = (acc[0] != last_acc[0] || acc[1] != last_acc[1] || acc[2] != last_acc[2]);
        if (maniac::config.theme_index != last_theme || accent_changed) {
            theme::apply(maniac::config.theme_index, acc);
            last_theme  = maniac::config.theme_index;
            last_acc[0] = acc[0]; last_acc[1] = acc[1]; last_acc[2] = acc[2];
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

#ifdef IMGUI_HAS_VIEWPORT
        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->GetWorkPos());
        ImGui::SetNextWindowSize(viewport->GetWorkSize());
        ImGui::SetNextWindowViewport(viewport->ID);
#else
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif

        body();

        maniac::config.tap_time              = max(0, maniac::config.tap_time);
        maniac::config.humanization_modifier = max(0, maniac::config.humanization_modifier);

        ImGui::EndFrame();

        g_pd3dDevice->SetRenderState(D3DRS_ZENABLE,           FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE,  FALSE);
        g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);

        g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,
            D3DCOLOR_RGBA(0, 0, 0, 255), 1.0f, 0);

        if (g_pd3dDevice->BeginScene() >= 0) {
            ImGui::Render();
            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            g_pd3dDevice->EndScene();
        }

        HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);
        if (result == D3DERR_DEVICELOST &&
            g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);
}

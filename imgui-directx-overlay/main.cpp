#include "include.h"

#include "menu.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LPDIRECT3D9 gpD3D = nullptr;
LPDIRECT3DDEVICE9 gpd3dDevice = nullptr;
D3DPRESENT_PARAMETERS gd3dpp;

HRESULT CreateDevice(HWND hWnd, D3DPRESENT_PARAMETERS* pParams)
{
    gpD3D = Direct3DCreate9(D3D_SDK_VERSION);
    if (!gpD3D)
        return E_FAIL;

    if (gpD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, pParams, &gpd3dDevice) < 0)
        return E_FAIL;

    return S_OK;
}

void CleanupDevice()
{
    if (gpd3dDevice)
    {
        gpd3dDevice->Release();
        gpd3dDevice = nullptr;
    }

    if (gpD3D)
    {
        gpD3D->Release();
        gpD3D = nullptr;
    }
}

void ResetDevice()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();

    if (gpd3dDevice->Reset(&gd3dpp) == D3DERR_INVALIDCALL)
        IM_ASSERT(0);

    ImGui_ImplDX9_CreateDeviceObjects();
}

LRESULT WINAPI D3DX9(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam))
        return 0;

    switch (msg)
    {
        case WM_SYSCOMMAND:
        {
            if ((wParam & 0xfff0) == SC_KEYMENU)
                return 0;

            break;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);

    const char* windowTitle = "Window";

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, D3DX9, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, windowTitle, nullptr };
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, windowTitle, windowTitle, WS_POPUP, 0, 0, desktop.right, desktop.bottom, nullptr, nullptr, wc.hInstance, nullptr);
    HDC hdc = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdc);

    BLENDFUNCTION blend;
    blend.AlphaFormat = AC_SRC_ALPHA;
    blend.BlendOp = AC_SRC_OVER;

    UpdateLayeredWindow(hwnd, hdc, nullptr, nullptr, hdcMem, nullptr, 0, &blend, ULW_ALPHA);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_COLORKEY);

    D3DPRESENT_PARAMETERS d3dparam;
    ZeroMemory(&d3dparam, sizeof(d3dparam));

    d3dparam.Windowed = TRUE;
    d3dparam.EnableAutoDepthStencil = TRUE;
    d3dparam.SwapEffect = D3DSWAPEFFECT_FLIP;
    d3dparam.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dparam.AutoDepthStencilFormat = D3DFMT_D24S8;
    d3dparam.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

    if (CreateDevice(hwnd, &d3dparam) < 0)
    {
        CleanupDevice();
        UnregisterClass(wc.lpszClassName, wc.hInstance);

        return 0;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    ImGui::CreateContext();
    DrawStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(gpd3dDevice);

    MSG message;
    ZeroMemory(&message, sizeof(message));

    while (message.message != WM_QUIT)
    {
        if (PeekMessage(&message, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&message);
            DispatchMessage(&message);

            continue;
        }

        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();

        ImGui::NewFrame();
        {
            DrawMenu();
        }
        ImGui::EndFrame();

        WindowClamp();

        if (gpd3dDevice->BeginScene() >= 0)
        {
            gpd3dDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0);

            ImGui::Render();

            ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
            gpd3dDevice->EndScene();
        }

        HRESULT result = gpd3dDevice->Present(nullptr, nullptr, nullptr, nullptr);
        if (result == D3DERR_DEVICELOST && gpd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
            ResetDevice();
    }

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDevice();

    DestroyWindow(hwnd);
    UnregisterClass(wc.lpszClassName, wc.hInstance);

    return 0;
}
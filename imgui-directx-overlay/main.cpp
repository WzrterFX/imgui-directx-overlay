#include "include.h"

#include "menu.hpp"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT message, WPARAM wParameter, LPARAM lParameter);

class Overlay
{
public:
    Overlay(HINSTANCE hInstance, UINT interval);
    ~Overlay();

    bool Push();
    void Show();

private:
    void Reboot();
    void Render();

    static LRESULT CALLBACK Procedure(HWND hwnd, UINT message, WPARAM wParameter, LPARAM lParameter);

    HINSTANCE hInstance;
    HWND hwnd;

    LPDIRECT3D9 pD3D;
    LPDIRECT3DDEVICE9 pD3DDevice;
    D3DPRESENT_PARAMETERS d3dpp;

    WNDCLASSEX windowClass;
    UINT interval;
};

Overlay::Overlay(HINSTANCE hInstance, UINT interval) : hInstance(hInstance), hwnd(nullptr), pD3D(nullptr), pD3DDevice(nullptr), interval(interval)
{
    ZeroMemory(&d3dpp, sizeof(d3dpp));

    windowClass =
    {
        .cbSize = sizeof(WNDCLASSEX),

        .style = CS_CLASSDC,

        .lpfnWndProc = Procedure,
        .hInstance = hInstance,

        .lpszClassName = "Window"
    };
}

Overlay::~Overlay()
{
    if (pD3DDevice)
        pD3DDevice->Release();

    if (pD3D)
        pD3D->Release();

    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (hwnd) DestroyWindow(hwnd);

    UnregisterClass(windowClass.lpszClassName, windowClass.hInstance);
}

bool Overlay::Push()
{
    if (!RegisterClassEx(&windowClass)) return FALSE;

    RECT desktop;
    GetWindowRect(GetDesktopWindow(), &desktop);

    hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TOPMOST, windowClass.lpszClassName, "Overlay", WS_POPUP, 0, 0, desktop.right, desktop.bottom, nullptr, nullptr, windowClass.hInstance, nullptr);
    if (!hwnd) return FALSE;

    BLENDFUNCTION blending
    {
        .BlendOp = AC_SRC_OVER,
        .AlphaFormat = AC_SRC_ALPHA
    };

    UpdateLayeredWindow(hwnd, GetDC(hwnd), nullptr, nullptr, CreateCompatibleDC(GetDC(hwnd)), nullptr, 0, &blending, ULW_ALPHA);
    SetLayeredWindowAttributes(hwnd, 0, 0, LWA_COLORKEY);

    d3dpp.Windowed = TRUE;
    d3dpp.EnableAutoDepthStencil = TRUE;

    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
    d3dpp.AutoDepthStencilFormat = D3DFMT_D24S8;

    d3dpp.PresentationInterval = interval;

    if (Direct3DCreate9(D3D_SDK_VERSION)->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3dpp, &pD3DDevice) < 0)
    {
        if (pD3DDevice)
            pD3DDevice->Release();

        if (pD3D)
            pD3D->Release();

        return FALSE;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    ImGui::CreateContext();
    DrawStyle();

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX9_Init(pD3DDevice);

    return TRUE;
}

void Overlay::Show()
{
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

        Render();
    }
}

void Overlay::Reboot()
{
    ImGui_ImplDX9_InvalidateDeviceObjects();
    if (pD3DDevice->Reset(&d3dpp) == D3DERR_INVALIDCALL)
        IM_ASSERT(0);

    ImGui_ImplDX9_CreateDeviceObjects();
}

void Overlay::Render()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    DrawMenu();

    ImGui::EndFrame();
    WindowClamp();

    if (pD3DDevice->BeginScene() >= 0)
    {
        pD3DDevice->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1, 0);
        ImGui::Render();

        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        pD3DDevice->EndScene();
    }

    if (pD3DDevice->Present(nullptr, nullptr, nullptr, nullptr) == D3DERR_DEVICELOST && pD3DDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
        Reboot();
}

LRESULT CALLBACK Overlay::Procedure(HWND hwnd, UINT message, WPARAM wParameter, LPARAM lParameter)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, message, wParameter, lParameter))
        return TRUE;

    switch (message)
    {
        case WM_SYSCOMMAND:
        {
            if ((wParameter & 0xfff0) == SC_KEYMENU)
                return EXIT_SUCCESS;

            break;
        }

        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return EXIT_SUCCESS;
        }
    }

    return DefWindowProc(hwnd, message, wParameter, lParameter);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    Overlay overlay(hInstance, D3DPRESENT_INTERVAL_DEFAULT);
    if (!overlay.Push())
        return EXIT_FAILURE;

    overlay.Show();
    return EXIT_SUCCESS;
}
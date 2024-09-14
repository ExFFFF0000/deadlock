#include "render.h"
#include <dwmapi.h>
#include <string>

bool render::CreateRenderTarget()
{
    ID3D11Texture2D* backBuffer;
    Data::SwapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    if (backBuffer != nullptr)
    {
        const HRESULT status = Data::Device->CreateRenderTargetView(backBuffer, nullptr, &Data::RenderTargetView);
        backBuffer->Release();
        return status == S_OK;
    }

    return false;
}

bool render::CreateDevice()
{
    DXGI_SWAP_CHAIN_DESC swapChainDescriptor;
    ZeroMemory(&swapChainDescriptor, sizeof(swapChainDescriptor));

    swapChainDescriptor.BufferCount = 2;
    swapChainDescriptor.BufferDesc.Width = 0;
    swapChainDescriptor.BufferDesc.Height = 0;
    swapChainDescriptor.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDescriptor.BufferDesc.RefreshRate.Numerator = 60;
    swapChainDescriptor.BufferDesc.RefreshRate.Denominator = 1;
    swapChainDescriptor.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    swapChainDescriptor.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDescriptor.OutputWindow = Data::MainWindow;
    swapChainDescriptor.SampleDesc.Count = 1;
    swapChainDescriptor.SampleDesc.Quality = 0;
    swapChainDescriptor.Windowed = TRUE;
    swapChainDescriptor.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    constexpr UINT createDeviceFlags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    if (D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &swapChainDescriptor, &Data::SwapChain, &Data::Device, &featureLevel, &Data::DeviceContext) != S_OK)
        return false;

    return CreateRenderTarget();
}

void render::CleanupRenderTarget()
{
    if (Data::RenderTargetView)
    {
        Data::RenderTargetView->Release();
        Data::RenderTargetView = nullptr;
    }
}

void render::CleanupDevice()
{
    CleanupRenderTarget();

    if (Data::SwapChain)
    {
        Data::SwapChain->Release();
        Data::SwapChain = nullptr;
    }

    if (Data::DeviceContext)
    {
        Data::DeviceContext->Release();
        Data::DeviceContext = nullptr;
    }

    if (Data::Device)
    {
        Data::Device->Release();
        Data::Device = nullptr;
    }
}



void render::loop(std::function<void()> const& func)
{
    const wchar_t* className = L"AppWindowClass";
    const WNDCLASSEXW wc = { sizeof(WNDCLASSEXW), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, className, nullptr };
    RegisterClassExW(&wc);

    const wchar_t* windowName = L"AppWindow";
    Data::MainWindow = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_NOACTIVATE, wc.lpszClassName, windowName, WS_POPUP, 0, 0, GetSystemMetrics(0), GetSystemMetrics(1), 0, 0, wc.hInstance, 0);
    SetLayeredWindowAttributes(Data::MainWindow, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margin = { -1 };
    DwmExtendFrameIntoClientArea(Data::MainWindow, &margin);
    if (!CreateDevice())
    {
        CleanupDevice();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        MessageBoxW(nullptr, L"Failed to create device!", L"Fatal error", MB_ICONERROR);
        return;
    }
    ShowWindow(Data::MainWindow, SW_SHOWDEFAULT);
    UpdateWindow(Data::MainWindow);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
   // Style::Apply();
    ImGuiStyle& style = ImGui::GetStyle();
    const HMONITOR monitor = MonitorFromWindow(Data::MainWindow, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfoW(monitor, &info);
    io.IniFilename = nullptr;
    io.Fonts->AddFontFromFileTTF(R"(C:\Windows\Fonts\simhei.ttf)", 16, 0, io.Fonts->GetGlyphRangesChineseFull());
    ImGui_ImplWin32_Init(Data::MainWindow);
    ImGui_ImplDX11_Init(Data::Device, Data::DeviceContext);
    //Manager::InitDefault();
    while (1)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
         //   if (msg.message == WM_QUIT)
              //  Global::ShouldExit = true;
        }
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        SetWindowLong(Data::MainWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        std::string fps = std::to_string(ImGui::GetIO().Framerate);
        ImGui::GetForegroundDrawList()->AddText(ImVec2(0, 0), ImColor(255, 255, 255), fps.c_str());
        func();
        ImGui::EndFrame();
        ImGui::Render();
        const ImVec4 clearColor = ImVec4(0.0f, 0.0f, 0.0f, 0.00f);
        const float clearColorWithAlpha[4] = { clearColor.x * clearColor.w, clearColor.y * clearColor.w, clearColor.z * clearColor.w, clearColor.w };
        Data::DeviceContext->OMSetRenderTargets(1, &Data::RenderTargetView, nullptr);
        Data::DeviceContext->ClearRenderTargetView(Data::RenderTargetView, clearColorWithAlpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        Data::SwapChain->Present(0, 0);
    }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI render::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;
    switch (msg)
    {
    case WM_SIZE:
        if (Data::Device != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            Data::SwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
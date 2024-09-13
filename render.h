#pragma once
#include <d3d11.h>

#include <ImGui/imgui_impl_dx11.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui.h>
#include <functional>


namespace render
{
    namespace Data
    {
        inline HWND MainWindow;
        inline ID3D11Device* Device;
        inline ID3D11DeviceContext* DeviceContext;
        inline IDXGISwapChain* SwapChain;
        inline ID3D11RenderTargetView* RenderTargetView;
    }

    bool CreateRenderTarget();
    bool CreateDevice();

    void CleanupRenderTarget();
    void CleanupDevice();

    void loop(std::function<void()> const& func);

    LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
}
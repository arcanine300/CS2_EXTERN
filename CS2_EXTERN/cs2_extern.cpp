#pragma comment(lib, "d3d11.lib")
#include <d3d11.h>
#include <dwmapi.h>
#include <chrono>
#include <thread>
#include <array>
#include <string>
#include <Windows.h>
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "framework.h"
#include "cs2_extern.h"
#include "MemoryManager.hpp"
#include "vec3.hpp"

//a2x dumper offsets
namespace CCSPlayerController {
    constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x750;
}

namespace CBasePlayerController {
    constexpr std::ptrdiff_t m_hPawn = 0x60C;
}

namespace C_BaseEntity {
    constexpr std::ptrdiff_t m_iHealth = 0x32C;
    constexpr std::ptrdiff_t m_iTeamNum = 0x3BF;
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x310;
}

namespace CGameSceneNode {
    constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8;
}

namespace client_dll {
    constexpr std::ptrdiff_t dwEntityList = 0x17CE6A0;
    constexpr std::ptrdiff_t dwLocalPlayerController = 0x181DC98;
    constexpr std::ptrdiff_t dwLocalPlayerPawn = 0x16D4F48;
    constexpr std::ptrdiff_t dwViewMatrix = 0x182CEA0;
}

MemoryManager* m = new MemoryManager();

HWND HijackNvidiaOverlay() {
    HWND OverlayWindow = FindWindowA("CEF-OSC-WIDGET", "NVIDIA GeForce Overlay");
    auto getInfo = GetWindowLongA(OverlayWindow, -20);
    auto changeAttributes = SetWindowLongA(OverlayWindow, -20, (LONG_PTR)(getInfo | 0x20));
    SetLayeredWindowAttributes(OverlayWindow, 0x000000, 0xFF, 0x02);
    SetWindowPos(OverlayWindow, HWND_TOPMOST, 0, 0, 0, 0, 0x0002 | 0x0001);

    RECT client_area{};
    GetClientRect(OverlayWindow, &client_area);
    RECT window_area{};
    GetWindowRect(OverlayWindow, &window_area);
    POINT diff{};
    ClientToScreen(OverlayWindow, &diff);
    const MARGINS margins{ window_area.left + (diff.x - window_area.left), window_area.top + (diff.y - window_area.top), client_area.right, client_area.bottom };
    DwmExtendFrameIntoClientArea(OverlayWindow, &margins);
    return OverlayWindow;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmd) {
    m->Init();
    m->InjectServer();
    HWND OverlayWindow = HijackNvidiaOverlay();
    HWND csgoWindow = FindWindowA(NULL, "Counter-Strike 2");
    DXGI_SWAP_CHAIN_DESC sd{}; //dx11 device setup
    sd.BufferDesc.RefreshRate.Numerator = 185U; //monitor hz
    sd.BufferDesc.RefreshRate.Denominator = 1U;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.SampleDesc.Count = 1U;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.BufferCount = 2U;
    sd.OutputWindow = OverlayWindow;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    constexpr D3D_FEATURE_LEVEL levels[2]{ D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    ID3D11Device* device{ nullptr };
    ID3D11DeviceContext* device_contxt{ nullptr };
    IDXGISwapChain* swap_chain{ nullptr };
    ID3D11RenderTargetView* render_target_view{ nullptr };
    D3D_FEATURE_LEVEL level{};
    D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0U, levels, 2U, D3D11_SDK_VERSION, &sd, &swap_chain, &device, &level, &device_contxt);
    ID3D11Texture2D* back_buffer{ nullptr };
    swap_chain->GetBuffer(0U, IID_PPV_ARGS(&back_buffer));

    if (!back_buffer)
        return 1;
    device->CreateRenderTargetView(back_buffer, nullptr, &render_target_view);

    ShowWindow(OverlayWindow, nCmd);
    UpdateWindow(OverlayWindow);
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = NULL;
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(OverlayWindow);
    ImGui_ImplDX11_Init(device, device_contxt);

    bool running = true;
    while (running) {

        if (!running)
            break;

        if (GetAsyncKeyState(VK_END) & 0x8000) //Exit key
            running = false;

        uintptr_t EntityList = m->ReadMem<uintptr_t>(m->moduleBase + client_dll::dwEntityList);
        uintptr_t list_entry = m->ReadMem<uintptr_t>(EntityList + 0x10);
        uintptr_t localPlayerController = m->ReadMem<uintptr_t>(m->moduleBase + client_dll::dwLocalPlayerController);
        uintptr_t localPlayerPawn = m->ReadMem<uintptr_t>(m->moduleBase + client_dll::dwLocalPlayerPawn);
        int localTeam = m->ReadMem<int>(localPlayerPawn + C_BaseEntity::m_iTeamNum);
        ViewMatrix view_matrix = m->ReadMem<ViewMatrix>(m->moduleBase + client_dll::dwViewMatrix);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        ImVec2 screenSize = ImGui::GetIO().DisplaySize;
     
        HWND CurrWindow = GetForegroundWindow();
        if (CurrWindow == csgoWindow) {
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(screenSize.x - 75.f, 5.f), ImColor(255, 255, 255), "CS2_EXTERN");

            for (int i = 1; i < 128; i++) { //i < 64 works on valve servers, custom servers might miss players as there can be blank entries
                uintptr_t currentController = m->ReadMem<uintptr_t>(list_entry + 120 * (i & 0x1FF));
                if (!currentController)
                    continue;

                uintptr_t playerNameAddress = m->ReadMem<uintptr_t>(currentController + CCSPlayerController::m_sSanitizedPlayerName);
                std::array<char, 100> className;
                if (playerNameAddress) {
                    className = m->ReadMem<std::array<char, 100>>(playerNameAddress);
                }
                
                std::string PlayerName;
                for (int i = 0; i < className.size(); i++) {
                    if (className.at(i) == '\0')
                        break;
                    PlayerName += className.at(i);
                }

                uint32_t playerPawnHandle = m->ReadMem<uint32_t>(currentController + CBasePlayerController::m_hPawn);
                uintptr_t list_entry2 = m->ReadMem<uintptr_t>(EntityList + 8 * ((playerPawnHandle & 0x7FFF) >> 9) + 16);
                if (!list_entry2)
                    continue;

                uintptr_t currentPawn = m->ReadMem<uintptr_t>(list_entry2 + 120 * (playerPawnHandle & 0x1FF));
                if (!currentPawn)
                    continue;

                uint32_t playerHealth = m->ReadMem<int32_t>(currentPawn + C_BaseEntity::m_iHealth);
                if (playerHealth > 100 || playerHealth <= 0)
                    continue;

                int TeamNum = m->ReadMem<int>(currentPawn + C_BaseEntity::m_iTeamNum);
                if (TeamNum == localTeam)
                    continue;

                uintptr_t gameSceneNode = m->ReadMem<uintptr_t>(currentPawn + C_BaseEntity::m_pGameSceneNode);
                vec3 Origin = m->ReadMem<vec3>(gameSceneNode + CGameSceneNode::m_vecAbsOrigin);
                if (Origin.x == 0.f && Origin.y == 0.f)
                    continue;

                vec3 headPos = { Origin.x , Origin.y , Origin.z + 72.f };
                vec3 paddingTop = { 0.f, 0.f, 4.f };  vec3 paddingBottom = { 0.f, 0.f, 6.f };
                vec3 top;  vec3 bottom; float h;  float w;
                if (worldToScreen(headPos + paddingTop, top, view_matrix) && worldToScreen(Origin - paddingBottom, bottom, view_matrix)) {
                    h = bottom.y - top.y;
                    w = h * 0.28f;
                    ImGui::GetBackgroundDrawList()->AddRect({ top.x - w, top.y }, { top.x + w, bottom.y }, ImColor(0, 220, 0), 0, ImDrawFlags_None, 1.f);
                    ImGui::GetBackgroundDrawList()->AddText(ImVec2(top.x + h * 0.33f, top.y - 10.f), ImColor(250, 250, 250), std::to_string(playerHealth).c_str());
                    ImGui::GetBackgroundDrawList()->AddText(ImVec2(top.x - h * 0.33f, bottom.y + 4.f), ImColor(250, 250, 250), PlayerName.c_str());
                }
            }
        }
        ImGui::Render();

        constexpr float color[4] = { 0.f, 0.f, 0.f, 0.f };
        device_contxt->OMSetRenderTargets(1U, &render_target_view, nullptr);
        device_contxt->ClearRenderTargetView(render_target_view, color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        swap_chain->Present(0U, 0U);
        std::this_thread::sleep_for(std::chrono::microseconds(500));
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
    ImGui::Render();
    constexpr float color[4] = { 0.f, 0.f, 0.f, 0.f };
    device_contxt->OMSetRenderTargets(1U, &render_target_view, nullptr);
    device_contxt->ClearRenderTargetView(render_target_view, color);
    swap_chain->Present(0U, 0U);
    std::this_thread::sleep_for(std::chrono::milliseconds(1));

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (swap_chain)
        swap_chain->Release();
    if (device_contxt)
        device_contxt->Release();
    if (device) 
        device->Release();
    if (render_target_view)
        render_target_view->Release();

    return 1;
}
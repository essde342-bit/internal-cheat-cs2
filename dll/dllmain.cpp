#include <Windows.h>
#include <thread>
#include <chrono>
#include "globals.h"
#include "memory.h"
#include "offsets.h"
#include "hooks.h"
#include "config.h"

void init_cheat(HMODULE hModule) {
    Globals::hModule = hModule;
    Config::init();
    
    HMODULE hClient = nullptr;
    while (!hClient) {
        hClient = GetModuleHandleA("client.dll");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    auto clientInfo = Memory::get_module_info("client.dll");
    Globals::clientBase = clientInfo.base;
    Globals::clientSize = clientInfo.size;
    
    Globals::entityList = Globals::clientBase + client_dll::dwEntityList;
    Globals::localPlayerController = Globals::clientBase + client_dll::dwLocalPlayerController;
    Globals::localPlayerPawn = Globals::clientBase + client_dll::dwLocalPlayerPawn;
    Globals::viewMatrix = Globals::clientBase + client_dll::dwViewMatrix;
    
    RECT rect;
    GetClientRect(GetForegroundWindow(), &rect);
    Globals::screenWidth = rect.right - rect.left;
    Globals::screenHeight = rect.bottom - rect.top;
    
    Hooks::init();
    
    while (!Globals::shouldUnload) {
        if (GetAsyncKeyState(VK_END) & 1) Globals::shouldUnload = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    Hooks::shutdown();
    FreeLibraryAndExitThread(hModule, 0);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(init_cheat), hModule, 0, nullptr);
    }
    return TRUE;
}

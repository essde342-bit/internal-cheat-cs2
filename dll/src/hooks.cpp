#include "hooks.h"
#include "render.h"
#include "esp.h"
#include "globals.h"
#include "config.h"
#include <iostream>

namespace Hooks {
    Present_t oPresent = nullptr;
    
    HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags) {
        static bool initialized = false;
        if (!initialized) {
            Render::init(pSwapChain);
            initialized = true;
        }
        
        Render::begin();
        ESP::render();
        Render::end();
        
        return oPresent(pSwapChain, SyncInterval, Flags);
    }
    
    bool init() {
        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = GetForegroundWindow();
        sd.SampleDesc.Count = 1;
        sd.Windowed = TRUE;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
        
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        IDXGISwapChain* swapChain = nullptr;
        
        D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
            D3D11_SDK_VERSION, &sd, &swapChain, &device, nullptr, &context
        );
        
        if (!swapChain) return false;
        
        void** vtable = *reinterpret_cast<void***>(swapChain);
        void* presentAddr = vtable[8];
        
        DWORD oldProtect;
        VirtualProtect(presentAddr, 12, PAGE_EXECUTE_READWRITE, &oldProtect);
        
        oPresent = reinterpret_cast<Present_t>(presentAddr);
        
        uint8_t jmp[] = { 0x48, 0xB8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xE0 };
        *reinterpret_cast<uint64_t*>(jmp + 2) = reinterpret_cast<uint64_t>(&hkPresent);
        memcpy(presentAddr, jmp, sizeof(jmp));
        
        VirtualProtect(presentAddr, 12, oldProtect, &oldProtect);
        
        swapChain->Release();
        device->Release();
        context->Release();
        
        return true;
    }
    
    void shutdown() {}
}

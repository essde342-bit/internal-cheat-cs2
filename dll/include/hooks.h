#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

namespace Hooks {
    bool init();
    void shutdown();
    
    typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
    extern Present_t oPresent;
}

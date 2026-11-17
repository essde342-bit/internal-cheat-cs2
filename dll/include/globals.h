#pragma once
#include <cstdint>
#include <Windows.h>

namespace Globals {
    extern HMODULE hModule;
    extern uintptr_t clientBase;
    extern size_t clientSize;
    
    extern uintptr_t entityList;
    extern uintptr_t localPlayerController;
    extern uintptr_t localPlayerPawn;
    extern uintptr_t viewMatrix;
    
    extern int screenWidth;
    extern int screenHeight;
    
    extern bool shouldUnload;
}

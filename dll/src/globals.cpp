#include "globals.h"

namespace Globals {
    HMODULE hModule = nullptr;
    uintptr_t clientBase = 0;
    size_t clientSize = 0;
    
    uintptr_t entityList = 0;
    uintptr_t localPlayerController = 0;
    uintptr_t localPlayerPawn = 0;
    uintptr_t viewMatrix = 0;
    
    int screenWidth = 1920;
    int screenHeight = 1080;
    
    bool shouldUnload = false;
}

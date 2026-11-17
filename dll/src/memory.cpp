#include "memory.h"

Memory::ModuleInfo Memory::get_module_info(const char* name) {
    MODULEINFO info = {};
    HMODULE hMod = GetModuleHandleA(name);
    if (!hMod) return { 0, 0 };
    
    GetModuleInformation(GetCurrentProcess(), hMod, &info, sizeof(MODULEINFO));
    return { reinterpret_cast<uintptr_t>(info.lpBaseOfDll), info.SizeOfImage };
}

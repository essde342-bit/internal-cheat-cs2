#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <string>
#include <vector>

DWORD get_process_id(const std::wstring& processName) {
    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(PROCESSENTRY32W);
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return 0;
    
    DWORD pid = 0;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, processName.c_str()) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    
    CloseHandle(snapshot);
    return pid;
}

bool inject_dll(DWORD pid, const std::string& dllPath) {
    HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!process) {
        std::cerr << "[-] Failed to open process. Error: " << GetLastError() << std::endl;
        return false;
    }
    
    size_t pathSize = dllPath.size() + 1;
    void* remoteMemory = VirtualAllocEx(process, nullptr, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remoteMemory) {
        std::cerr << "[-] Failed to allocate memory. Error: " << GetLastError() << std::endl;
        CloseHandle(process);
        return false;
    }
    
    if (!WriteProcessMemory(process, remoteMemory, dllPath.c_str(), pathSize, nullptr)) {
        std::cerr << "[-] Failed to write memory. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(process, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }
    
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, 
        reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA")),
        remoteMemory, 0, nullptr);
    
    if (!thread) {
        std::cerr << "[-] Failed to create remote thread. Error: " << GetLastError() << std::endl;
        VirtualFreeEx(process, remoteMemory, 0, MEM_RELEASE);
        CloseHandle(process);
        return false;
    }
    
    WaitForSingleObject(thread, INFINITE);
    
    VirtualFreeEx(process, remoteMemory, 0, MEM_RELEASE);
    CloseHandle(thread);
    CloseHandle(process);
    
    return true;
}

int main() {
    std::cout << "=== CS2 Internal Injector ===" << std::endl;
    std::cout << "Looking for cs2.exe..." << std::endl;
    
    DWORD pid = get_process_id(L"cs2.exe");
    if (!pid) {
        std::cerr << "[-] cs2.exe not found. Make sure the game is running." << std::endl;
        system("pause");
        return 1;
    }
    
    std::cout << "[+] Found cs2.exe (PID: " << pid << ")" << std::endl;
    
    char dllPath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, dllPath, MAX_PATH);
    std::string pathStr(dllPath);
    size_t lastSlash = pathStr.find_last_of("\\/");
    if (lastSlash != std::string::npos) {
        pathStr = pathStr.substr(0, lastSlash + 1);
    }
    pathStr += "cs2_internal.dll";
    
    std::cout << "[+] Injecting: " << pathStr << std::endl;
    
    if (inject_dll(pid, pathStr)) {
        std::cout << "[+] Injection successful!" << std::endl;
    } else {
        std::cerr << "[-] Injection failed." << std::endl;
    }
    
    system("pause");
    return 0;
}
